#pragma once

// #include "predictor.h"
#include "thread_state.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <boost/filesystem.hpp>

// If a high-criticality job 𝐽𝑖 does not complete within its virtual deadline 𝐷′,
// the system transitions into the critical-state. All low-criticality jobs can be abandoned
// and future jobs of low-criticality tasks need not be admitted.
// The scheduler now executes all jobs according to their critical-state mapping 𝑆.
// That is, all high-utilization tasks are now allocated 𝑛 dedicated cores and
// scheduled using a work-conserving scheduler.

namespace srsran {

struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  // int                                   worker_id;
  // long long                             task_block_time;
  // int                                   queue_len;
  unsigned current_cpu_index_point;
  unsigned idle_sum_increase;
  unsigned idle_sum_decrease;
};

inline void write_to_csv(std::string filename, const std::vector<LogEntry>& log_buffer)
{
  std::ofstream csv_file(filename, std::ios::out | std::ios::app);
  if (!csv_file.is_open()) {
    throw std::runtime_error("Failed to open CSV file");
  }

  for (const auto& entry : log_buffer) {
    auto time_stamp = std::chrono::duration_cast<std::chrono::microseconds>(entry.timestamp.time_since_epoch()).count();
    // csv_file << time_stamp << "," << entry.worker_id << "," << entry.task_block_time << "," << entry.queue_len << ","
    //          << entry.current_cpu_index_point << "," << entry.idle_sum_increase << "," << entry.idle_sum_decrease
    //          << "\n";
    csv_file << time_stamp << "," << entry.current_cpu_index_point << "," << entry.idle_sum_increase << ","
             << entry.idle_sum_decrease << "\n";
  }

  csv_file.flush(); // Ensure the data is written to disk
}

class scheduler_zwh
{
public:
  scheduler_zwh()
  {
    feature = std::vector<float>(9, 0);
    pos = 0;
    stop_flag.store(false);
    transmit_flag.store(false);
  }

  void update_feature(std::vector<float> feature_)
  {
    for(int i = 0; i < 7; i++){
      feature[i + 1] = feature_[i];
    }
  }

  void insert_bs(float bs)
  {
    feature[0] = bs;
    transmit_flag.store(true);
  }

  static scheduler_zwh& getInstance()
  {
    static scheduler_zwh instance;
    return instance;
  }

  std::function<void()> DL_control(unsigned                      nof_workers,
                                   std::function<void(unsigned)> sleep_thread,
                                   std::function<void(unsigned)> wake_thread,
                                   std::function<unsigned(void)> get_nof_pending_tasks,
                                   std::string                   pool_name)
  {
    return [this, nof_workers, sleep_thread, wake_thread, get_nof_pending_tasks, pool_name]() {
      auto     current                 = std::chrono::system_clock::now();
      auto     log_current             = std::chrono::system_clock::now();
      auto     event_current           = std::chrono::system_clock::now();
      auto     debug_print_current     = std::chrono::system_clock::now();
      unsigned current_cpu_index_point = (nof_workers == 1 ? nof_workers : nof_workers / 2);

      sem_t* sem_ready = sem_open(SEM_READY_NAME, O_CREAT, 0666, 0);
      sem_t* sem_done = sem_open(SEM_DONE_NAME, O_CREAT, 0666, 0);
      
      // 创建共享内存
      int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      ftruncate(shm_fd, SHM_SIZE);
      float* shared_data = static_cast<float*>(mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

      std::vector<LogEntry> log_buffer; // Buffer to store log entries
      const unsigned        BUFFER_SIZE = 1000;

      // 初始化CSV文件
      std::string          log_file_name = "dl_task_log.csv";
      static std::ofstream csv_file(log_file_name, std::ios::out | std::ios::trunc);
      if (!csv_file.is_open()) {
        throw std::runtime_error("Failed to open CSV file");
      }
      csv_file << "timestamp,current_cpu_index_point,idle_sum_increase,"
                  "idle_sum_decrease\n "; // 写入表头
      csv_file.close();                   // 关闭文件，后续由 write_to_csv 以追加模式打开

      // 初始化任务完成时间的CSV文件
      std::string          log_file_name2 = "dl_task_finished_time.csv";
      static std::ofstream csv_file2(log_file_name2, std::ios::out | std::ios::trunc);
      if (!csv_file2.is_open()) {
        throw std::runtime_error("Failed to open CSV file");
      }
      csv_file2 << "timestamp,duration\n";        // 写入表头
      csv_file2.close();                          // 关闭文件，后续由 write_to_csv 以追加模式打开
      const unsigned block_record_frequency = 2;  // 记录频率
      const unsigned cpu_adjust_frequency   = 24; // CPU 调整频率
      const unsigned cpu_reclaim_frequency = cpu_adjust_frequency * 500; // CPU 回收频率，目前是12000us
      const unsigned cpu_increase_frequency = cpu_adjust_frequency * 200; // CPU 增加频率
      const unsigned log_frequency          = 2;                          // 日志记录频率

      // 修改：将窗口分为两个不同的窗口，一个用于CPU增加，一个用于CPU减少
      const unsigned window_size_increase = 500 * 1.33; // CPU增加窗口大小
      const unsigned window_size_decrease = 2300 * 1.2; // CPU减少窗口大小
      const unsigned num_records_increase = window_size_increase / block_record_frequency;
      const unsigned num_records_decrease = window_size_decrease / block_record_frequency;

      // 修改F_min和F_max分别只依赖于对应的窗口大小
      // F_min控制cpu增加，F_min越大，说明越容易增加CPU
      // F_max控制CPU减少，F_max越小，说明越容易减少CPU
      unsigned F_min = 0.28 * window_size_increase * 1.45;
      unsigned F_max = 0.45 * window_size_decrease * 1.1;

      std::vector<unsigned> idle_counts_increase(num_records_increase, 0); // CPU增加窗口环形缓冲区
      unsigned              idle_sum_increase     = 0;                     // CPU增加窗口内的累积值
      unsigned              record_index_increase = 0;                     // CPU增加窗口当前索引

      std::vector<unsigned> idle_counts_decrease(num_records_decrease, 0); // CPU减少窗口环形缓冲区
      unsigned              idle_sum_decrease     = 0;                     // CPU减少窗口内的累积值
      unsigned              record_index_decrease = 0;                     // CPU减少窗口当前索引

      unsigned idle_count = 0; // 队列为空的次数

      auto now = std::chrono::system_clock::now();
      auto last_reclaim_cpu_time  = std::chrono::system_clock::now(); // 用于记录上次回收cpu的时间
      auto last_increase_cpu_time = std::chrono::system_clock::now(); // 用于记录上次增加cpu的时间
      auto duration               = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();
      auto log_duration           = std::chrono::duration_cast<std::chrono::microseconds>(now - log_current).count();
      auto event_duration         = std::chrono::duration_cast<std::chrono::microseconds>(now - event_current).count();
      auto debug_print_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(now - debug_print_current).count();
      while (!stop_flag.load(std::memory_order_relaxed)) {
        if (pool_name.find("up_phy_dl") != std::string::npos) {
          now = std::chrono::system_clock::now();
          duration       = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();
          log_duration   = std::chrono::duration_cast<std::chrono::microseconds>(now - log_current).count();
          event_duration = std::chrono::duration_cast<std::chrono::microseconds>(now - event_current).count();
          debug_print_duration =
              std::chrono::duration_cast<std::chrono::microseconds>(now - debug_print_current).count();

          // 用于debug在终端输出控制频率
          if (debug_print_duration >= 1000000) {
            debug_print_current = now;
          }
          if (event_duration >= block_record_frequency) {
            if (get_nof_pending_tasks() == 0) {
              idle_count = 1;
            } else {
              idle_count = 0;
            }
            // 更新CPU增加窗口
            idle_sum_increase -= idle_counts_increase[record_index_increase];
            idle_counts_increase[record_index_increase] = idle_count;
            idle_sum_increase += idle_count;
            record_index_increase = (record_index_increase + 1) % num_records_increase;

            // 更新CPU减少窗口
            idle_sum_decrease -= idle_counts_decrease[record_index_decrease];
            idle_counts_decrease[record_index_decrease] = idle_count;
            idle_sum_decrease += idle_count;
            record_index_decrease = (record_index_decrease + 1) % num_records_decrease;
            event_current         = now;
          }

          // 记录日志
          if (log_duration >= log_frequency) {
            log_buffer.push_back({now, current_cpu_index_point, idle_sum_increase, idle_sum_decrease});
            log_current = now;
          }
          if (log_buffer.size() >= BUFFER_SIZE && is_log2file) {
            // 使用 std::async 异步调用文件写入操作
            std::async(std::launch::async, [log_file_name, log_buffer_copy = std::vector<LogEntry>(log_buffer)]() {
              write_to_csv(log_file_name, log_buffer_copy); // 异步写入 CSV
            });
            log_buffer.clear(); // 清空缓冲区
          }
          if (duration >= cpu_adjust_frequency) {
            // 将队列为空的次数控制在 [F_min, F_max]
            if (idle_sum_increase < F_min && current_cpu_index_point < nof_workers) {
              // 增加 CPU 核数
              auto increase_cpu_time = std::chrono::system_clock::now();
              if (std::chrono::duration_cast<std::chrono::microseconds>(increase_cpu_time - last_increase_cpu_time)
                      .count() > cpu_increase_frequency) {
                last_increase_cpu_time = increase_cpu_time;
                wake_thread(current_cpu_index_point);
                ++current_cpu_index_point;
              }
            } else if (idle_sum_decrease > F_max && current_cpu_index_point > 1) {
              // 减少 CPU 核数
              auto reclaim_cpu_time = std::chrono::system_clock::now();
              if (std::chrono::duration_cast<std::chrono::microseconds>(reclaim_cpu_time - last_reclaim_cpu_time)
                      .count() > cpu_reclaim_frequency) {
                last_reclaim_cpu_time = reclaim_cpu_time;
                sleep_thread(--current_cpu_index_point);
              }
            }
            current = now;
          }
          if(transmit_flag.load()){
            feature[8] = current_cpu_index_point;
            for(int i = pos * 9; i < (pos + 1) * 9; i++){
              shared_data[i] = feature[i - pos * 9];
              // std::cout << shared_data[i] << " ";
            }
            // std::cout << "\n";
            pos = (pos + 1) % 2000;
            shared_data[ARRAY_SIZE] = pos;
            transmit_flag.store(false);
          }
          sem_post(sem_ready);
        }
      }

      if (!log_buffer.empty() && is_log2file) {
        // 使用 std::async 异步调用文件写入操作
        std::async(std::launch::async, [log_file_name, log_buffer_copy = std::vector<LogEntry>(log_buffer)]() {
          write_to_csv(log_file_name, log_buffer_copy); // 异步写入 CSV
        });
        log_buffer.clear(); // 清空缓冲区
      }

      munmap(shared_data, SHM_SIZE);
      close(shm_fd);
      shm_unlink(SHM_NAME);
      sem_close(sem_ready);
      sem_close(sem_done);
      sem_unlink(SEM_READY_NAME);
      sem_unlink(SEM_DONE_NAME);
    };
  }

  std::atomic<bool> stop_flag;
  const bool        is_log2file = true;
  int pos;
  std::vector<float> feature;
  std::atomic<bool> transmit_flag;

  const char* SHM_NAME = "/cpp_py_array";
  const char* SEM_READY_NAME = "/cpp_py_sem_ready";
  const char* SEM_DONE_NAME = "/cpp_py_sem_done";
  const size_t ARRAY_SIZE = 2000 * 9;  // 数组大小
  const size_t SHM_SIZE = sizeof(float) * (ARRAY_SIZE + 1);   
};

} // namespace srsran