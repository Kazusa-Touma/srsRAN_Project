#ifndef SRSRAN_SCHEDULER_ZWH_H
#define SRSRAN_SCHEDULER_ZWH_H

// #include "predictor.h"
#include "thread_state.h"
#include "srsran/support/executors/task_worker_pool.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// If a high-criticality job 𝐽𝑖 does not complete within its virtual deadline 𝐷′,
// the system transitions into the critical-state. All low-criticality jobs can be abandoned
// and future jobs of low-criticality tasks need not be admitted.
// The scheduler now executes all jobs according to their critical-state mapping 𝑆.
// That is, all high-utilization tasks are now allocated 𝑛 dedicated cores and
// scheduled using a work-conserving scheduler.

namespace srsran {

struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  int                                   worker_id;
  long long                             task_block_time;
  int                                   queue_len;
  unsigned                              current_cpu_index_point;
  unsigned                              idle_sum_increase;
  unsigned                              idle_sum_decrease;
};

inline void write_to_csv(std::string filename, const std::vector<LogEntry>& log_buffer)
{
  std::ofstream csv_file(filename, std::ios::out | std::ios::app);
  if (!csv_file.is_open()) {
    throw std::runtime_error("Failed to open CSV file");
  }

  for (const auto& entry : log_buffer) {
    auto time_stamp = std::chrono::duration_cast<std::chrono::microseconds>(entry.timestamp.time_since_epoch()).count();
    csv_file << time_stamp << "," << entry.worker_id << "," << entry.task_block_time << "," << entry.queue_len << ","
             << entry.current_cpu_index_point << "," << entry.idle_sum_increase << "," << entry.idle_sum_decrease
             << "\n";
  }

  csv_file.flush(); // Ensure the data is written to disk
}

class scheduler_zwh
{
public:
  scheduler_zwh()
  {
    // pred = std::make_unique<predictor>();
    stop_flag.store(false);
  }

  static scheduler_zwh& getInstance()
  {
    static scheduler_zwh instance;
    return instance;
  }

  template <concurrent_queue_policy QueuePolicy>
  std::function<void()> UL_control(unsigned                      nof_workers,
                                   std::function<void(unsigned)> sleep_thread,
                                   std::function<void(unsigned)> wake_thread,
                                   std::function<unsigned(void)> get_nof_pending_tasks,
                                   std::string                   pool_name)
  {
    return [this, nof_workers, sleep_thread, wake_thread, get_nof_pending_tasks, pool_name]() {
      auto     current                 = std::chrono::system_clock::now();
      unsigned current_cpu_index_point = (nof_workers == 1 ? nof_workers : nof_workers / 2);
      // unsigned current_cpu_index_point = 4;

      std::vector<LogEntry> log_buffer; // Buffer to store log entries
      const unsigned        BUFFER_SIZE = 1000;

      // 初始化CSV文件
      std::string          log_file_name = "ul_task_log.csv";
      static std::ofstream csv_file(log_file_name, std::ios::out | std::ios::trunc);
      if (!csv_file.is_open()) {
        throw std::runtime_error("Failed to open CSV file");
      }
      csv_file << "timestamp,worker_id,task_block_time,queue_len,current_cpu_index_point,idle_sum\n"; // 写入表头
      csv_file.close(); // 关闭文件，后续由 write_to_csv 以追加模式打开

      // 初始化任务完成时间的CSV文件
      std::string          log_file_name2 = "ul_task_finished_time.csv";
      static std::ofstream csv_file2(log_file_name2, std::ios::out | std::ios::trunc);
      if (!csv_file2.is_open()) {
        throw std::runtime_error("Failed to open CSV file");
      }
      csv_file2 << "timestamp,duration\n"; // 写入表头
      csv_file2.close();                   // 关闭文件，后续由 write_to_csv 以追加模式打开

      const unsigned block_record_frequency = 2;    // 记录频率
      const unsigned cpu_adjust_frequency   = 20;   // CPU 调整频率
      const unsigned window_size            = 3000; // 窗口大小
      const unsigned num_records            = window_size / block_record_frequency;

      std::vector<unsigned> idle_counts(num_records, 0); // 环形缓冲区，存储记录的值
      unsigned              idle_sum     = 0;            // 当前窗口内的累积值
      unsigned              record_index = 0;            // 环形缓冲区当前索引

      unsigned idle_count = 0; // 队列为空的次数
      unsigned F_min = 400, F_max = 600;

      auto now      = std::chrono::system_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();
      while (!stop_flag.load(std::memory_order_relaxed)) {
        now      = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();

        // 每 2us 记录一次队列为空的次数
        if (duration % block_record_frequency == 0 && get_nof_pending_tasks() == 0) {
          idle_count = 1;
        } else {
          idle_count = 0;
        }

        if (duration >= cpu_adjust_frequency) {
          if (pool_name.find("pusch") != std::string::npos) {
            // 更新环形缓冲区
            idle_sum -= idle_counts[record_index];
            idle_counts[record_index] = idle_count;
            idle_sum += idle_count;
            record_index = (record_index + 1) % num_records;
            // fmt::print("pusch idle_sum: {}\n", idle_sum);

            // 记录日志
            for (int i = 0; i < int(current_cpu_index_point); i++) {
              long block_time = pusch_thread_state::getInstance().getBlockTime(i);
              log_buffer.push_back(
                  {now, i, block_time, int(get_nof_pending_tasks()), current_cpu_index_point, idle_sum});
              if (log_buffer.size() >= BUFFER_SIZE) {
                write_to_csv(log_file_name, log_buffer); // 写入 CSV 并清空缓冲区
              }
            }

            // 将队列为空的次数控制在 [F_min, F_max]
            if (idle_sum < F_min && current_cpu_index_point < nof_workers) {
              // 增加 CPU 核数
              // wake_thread(current_cpu_index_point);
              // ++current_cpu_index_point;
            } else if (idle_sum > F_max && current_cpu_index_point > 1) {
              // 减少 CPU 核数
              // sleep_thread(--current_cpu_index_point);
            }
          }
          current = now;
        }
      }

      if (!log_buffer.empty()) {
        write_to_csv(log_file_name, log_buffer); // 写入剩余日志
      }
    };
  }

  template <concurrent_queue_policy QueuePolicy>
  std::function<void()> DL_control(unsigned                      nof_workers,
                                   std::function<void(unsigned)> sleep_thread,
                                   std::function<void(unsigned)> wake_thread,
                                   std::function<unsigned(void)> get_nof_pending_tasks,
                                   std::string                   pool_name)
  {
    return [this, nof_workers, sleep_thread, wake_thread, get_nof_pending_tasks, pool_name]() {
      auto     current                 = std::chrono::system_clock::now();
      auto     log_current             = std::chrono::system_clock::now();
      unsigned current_cpu_index_point = (nof_workers == 1 ? nof_workers : nof_workers / 2);
      // unsigned current_cpu_index_point = 4;

      std::vector<LogEntry> log_buffer; // Buffer to store log entries
      const unsigned        BUFFER_SIZE = 1000;

      // 初始化CSV文件
      std::string          log_file_name = "dl_task_log.csv";
      static std::ofstream csv_file(log_file_name, std::ios::out | std::ios::trunc);
      if (!csv_file.is_open()) {
        throw std::runtime_error("Failed to open CSV file");
      }
      csv_file << "timestamp,worker_id,task_block_time,queue_len,current_cpu_index_point,idle_sum_increase,"
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
      // const unsigned cpu_adjust_frequency = 15; // CPU 调整频率
      // const unsigned cpu_reclaim_frequency  = 40000; // CPU 回收频率
      // const unsigned cpu_reclaim_frequency  = cpu_adjust_frequency * 1500; // CPU 回收频率，目前是36000us
      const unsigned cpu_reclaim_frequency = cpu_adjust_frequency * 500; // CPU 回收频率，目前是36000us
      // const unsigned cpu_increase_frequency = 400;                        // CPU 增加频率
      const unsigned cpu_increase_frequency = cpu_adjust_frequency * 20; // CPU 增加频率
      const unsigned log_frequency          = 2;                         // 日志记录频率

      // 修改：将窗口分为两个不同的窗口，一个用于CPU增加，一个用于CPU减少
      const unsigned window_size_increase = 500;  // CPU增加窗口大小
      const unsigned window_size_decrease = 2300; // CPU减少窗口大小
      // const unsigned window_size_increase = 21 * cpu_adjust_frequency; // CPU增加窗口大小
      // const unsigned window_size_decrease = 96 * cpu_adjust_frequency; // CPU减少窗口大小
      const unsigned num_records_increase = window_size_increase / block_record_frequency;
      const unsigned num_records_decrease = window_size_decrease / block_record_frequency;

      // 修改F_min和F_max分别只依赖于对应的窗口大小
      unsigned F_min = 0.33 * window_size_increase;
      unsigned F_max = 0.45 * window_size_decrease;
      // unsigned F_max = 0.4 * window_size_decrease;

      std::vector<unsigned> idle_counts_increase(num_records_increase, 0); // CPU增加窗口环形缓冲区
      unsigned              idle_sum_increase     = 0;                     // CPU增加窗口内的累积值
      unsigned              record_index_increase = 0;                     // CPU增加窗口当前索引

      std::vector<unsigned> idle_counts_decrease(num_records_decrease, 0); // CPU减少窗口环形缓冲区
      unsigned              idle_sum_decrease     = 0;                     // CPU减少窗口内的累积值
      unsigned              record_index_decrease = 0;                     // CPU减少窗口当前索引

      unsigned idle_count = 0; // 队列为空的次数

      auto now                    = std::chrono::system_clock::now();
      auto log_now                = std::chrono::system_clock::now();
      auto last_reclaim_cpu_time  = std::chrono::system_clock::now(); // 用于记录上次回收cpu的时间
      auto last_increase_cpu_time = std::chrono::system_clock::now(); // 用于记录上次增加cpu的时间
      auto duration               = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();
      auto log_duration = std::chrono::duration_cast<std::chrono::microseconds>(log_now - log_current).count();
      while (!stop_flag.load(std::memory_order_relaxed)) {
        now          = std::chrono::system_clock::now();
        log_now      = std::chrono::system_clock::now();
        duration     = std::chrono::duration_cast<std::chrono::microseconds>(now - current).count();
        log_duration = std::chrono::duration_cast<std::chrono::microseconds>(log_now - log_current).count();

        // 每 2us 记录一次队列为空的次数
        if (duration % block_record_frequency == 0 && get_nof_pending_tasks() == 0) {
          idle_count = 1;
        } else {
          idle_count = 0;
        }

        if (duration >= cpu_adjust_frequency) {
          if (pool_name.find("up_phy_dl") != std::string::npos) {
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

            // 记录日志
            if (log_duration >= log_frequency) {
              for (int i = 0; i < int(current_cpu_index_point); i++) {
                long block_time = dl_thread_state::getInstance().getBlockTime(i);
                log_buffer.push_back({now,
                                      i,
                                      block_time,
                                      int(get_nof_pending_tasks()),
                                      current_cpu_index_point,
                                      idle_sum_increase,
                                      idle_sum_decrease});
              }
            }
            if (log_buffer.size() >= BUFFER_SIZE) {
              // 使用 std::async 异步调用文件写入操作
              std::async(std::launch::async, [log_file_name, log_buffer_copy = std::vector<LogEntry>(log_buffer)]() {
                write_to_csv(log_file_name, log_buffer_copy); // 异步写入 CSV
              });
              log_buffer.clear(); // 清空缓冲区
            }

            /* start - 用户测试不同的core固定时的性能 */

            // unsigned int pin_cpu = 1;

            // if (current_cpu_index_point > pin_cpu) {
            //   sleep_thread(--current_cpu_index_point);
            // }
            // if (current_cpu_index_point < pin_cpu) {
            //   wake_thread(current_cpu_index_point);
            //   ++current_cpu_index_point;
            // }
            /* end - 用户测试不同的core固定时的性能 */
          }

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
      }

      if (!log_buffer.empty()) {
        // 使用 std::async 异步调用文件写入操作
        std::async(std::launch::async, [log_file_name, log_buffer_copy = std::vector<LogEntry>(log_buffer)]() {
          write_to_csv(log_file_name, log_buffer_copy); // 异步写入 CSV
        });
        log_buffer.clear(); // 清空缓冲区
      }
    };
  }

  std::atomic<bool> stop_flag;
};

} // namespace srsran
#endif