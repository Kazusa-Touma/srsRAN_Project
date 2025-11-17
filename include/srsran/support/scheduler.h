#ifndef SRSRAN_SCHEDULER_H
#define SRSRAN_SCHEDULER_H

#include "predictor.h"
#include "timestamp_logger.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <boost/filesystem.hpp>

namespace srsran {

class CPU_controller{
public:
  std::atomic<bool> stop_flag;

  static CPU_controller& getInstance()
  {
    static CPU_controller instance;
    return instance;
  }

  void update_feature(std::vector<double> feature_)
  {
    for(int i = 0; i < 8; i++){
      feature[i] = feature_[i];
    }
  }

  void insert_bs(unsigned int bs)
  {
    feature[8] = static_cast<double>(bs);
    transmit_flag.store(true);
  }

  std::function<void()> check_status(unsigned nof_workers)
  {
    return [this, nof_workers]() {
      // std::ofstream csv_file("task log.csv", std::ios::out);
      // csv_file << "timestamp,core_num,num1,num2,start_point\n";
      sem_t* sem_ready = sem_open(SEM_READY_NAME, O_CREAT, 0666, 0);
      sem_t* sem_done = sem_open(SEM_DONE_NAME, O_CREAT, 0666, 0);
      
      // 创建共享内存
      int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      ftruncate(shm_fd, SHM_SIZE);
      double* shared_data = static_cast<double*>(mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
      
      while (!stop_flag.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        if(transmit_flag.load()){
          for(int i = pos * 9; i < (pos + 1) * 9; i++){
            shared_data[i] = feature[i - pos * 9];
            std::cout << feature[i - pos * 9] << " ";
          }
          std::cout << "\n";
          shared_data[ARRAY_SIZE] = pos;
          pos = (pos + 1) % 50;
          transmit_flag.store(false);
        }
        sem_post(sem_ready);
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

private:
  int pos, cnt;
  std::vector<double> feature;
  std::atomic<bool> sched_flag;
  std::atomic<bool> transmit_flag;

  CPU_controller()
  {
    feature = std::vector<double>(9, 0);
    pos = 0;
    stop_flag.store(false);
    transmit_flag.store(false);
  }

  const char* SHM_NAME = "/cpp_py_array";
  const char* SEM_READY_NAME = "/cpp_py_sem_ready";
  const char* SEM_DONE_NAME = "/cpp_py_sem_done";
  const size_t ARRAY_SIZE = 450;  // 数组大小
  const size_t SHM_SIZE = sizeof(double) * (ARRAY_SIZE + 1);  // 数组+返回值
};

} // namespace srsran
#endif