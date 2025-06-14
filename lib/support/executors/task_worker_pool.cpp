/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsran/support/executors/task_worker_pool.h"
#include <future>
#include <mutex>

std::mutex incre_mutex;

using namespace srsran;

detail::base_worker_pool::base_worker_pool(unsigned                              nof_workers_,
                                           const std::string&                    worker_pool_name,
                                           const std::function<void()>&          run_tasks_job,
                                           os_thread_realtime_priority           prio,
                                           span<const os_sched_affinity_bitmask> cpu_masks) :
  pool_name(worker_pool_name)
{
  worker_threads.reserve(nof_workers_);

  if (cpu_masks.size() > 1) {
    // An array with a single mask is allowed, otherwise the number of masks must be equal to the number of workers.
    report_error_if_not(cpu_masks.size() == nof_workers_, "Wrong array of CPU masks provided");
  }

  // unsigned actual_workers = nof_workers_ / 2 ? nof_workers_ / 2 : 1;
  for (unsigned i = 0; i < nof_workers_; i++) {
    // is_yield.push_back(!(i >= actual_workers && (worker_pool_name.find("up_phy_dl") != std::string::npos ||
    // worker_pool_name.find("pusch") != std::string::npos)));
    is_yield.push_back(true);
    cv.emplace_back(new std::condition_variable());
    mtx.emplace_back(new std::mutex());
  }

  // Task dispatched to workers of the pool.
  for (unsigned i = 0; i != nof_workers_; ++i) {
    if (cpu_masks.empty()) {
      worker_threads.emplace_back(fmt::format("{}#{}", worker_pool_name, i), prio, run_tasks_job);
    } else {
      // Check whether a single mask for all workers should be used.
      os_sched_affinity_bitmask cpu_mask = (cpu_masks.size() == 1) ? cpu_masks[0] : cpu_masks[i];
      worker_threads.emplace_back(fmt::format("{}#{}", worker_pool_name, i), prio, cpu_mask, run_tasks_job);
    }
  }
}

bool detail::base_worker_pool::is_in_thread_pool() const
{
  return std::any_of(worker_threads.begin(),
                     worker_threads.end(),
                     [id = std::this_thread::get_id()](const unique_thread& t) { return t.get_id() == id; });
}

unsigned detail::base_worker_pool::update_id()
{
  std::lock_guard<std::mutex> lock(incre_mutex);
  id++;
  // fmt::print("update_id is called by <{}>, the current id is ---> {}\n", this->pool_name, id);
  return id;
}

// //////////////////////////

priority_task_worker_pool::priority_task_worker_pool(std::string                           worker_pool_name,
                                                     unsigned                              nof_workers_,
                                                     span<const concurrent_queue_params>   queue_params,
                                                     std::chrono::microseconds             wait_sleep_time,
                                                     os_thread_realtime_priority           prio,
                                                     span<const os_sched_affinity_bitmask> cpu_masks) :
  detail::base_priority_task_queue(queue_params, wait_sleep_time),
  detail::base_worker_pool(nof_workers_, std::move(worker_pool_name), create_pop_loop_task(), prio, cpu_masks)
{
  report_fatal_error_if_not(nof_workers_ > 0, "Number of workers of a thread pool must be greater than 0");
  srsran_assert(queue_params.size() >= 2, "Number of queues in a prioritized thread pool must be greater than 1");
}

priority_task_worker_pool::~priority_task_worker_pool()
{
  stop();
}

void priority_task_worker_pool::stop()
{
  unsigned count = 0;
  for (unique_thread& w : worker_threads) {
    if (w.running()) {
      this->queue.request_stop();
      w.join();
      this->logger.info("Task worker \"{}\" finished.", worker_threads[count].get_name());
    }
    count++;
  }
}

void priority_task_worker_pool::wait_pending_tasks()
{
  std::mutex              mutex;
  std::condition_variable cvar_all_sync, cvar_caller_return;

  // Check if there are workers still running.
  unsigned workers_running = 0;
  for (unsigned i = 0; i != nof_workers(); ++i) {
    if (worker_threads[i].running()) {
      workers_running++;
    }
  }
  if (workers_running == 0) {
    // If no worker is running, return immediately.
    return;
  }

  // This will block all workers until all of them are running the enqueued task.
  unsigned count_workers_not_sync = workers_running;
  unsigned counter_caller         = workers_running;
  for (unsigned i = 0; i != workers_running; ++i) {
    if (not this->queue.push_blocking(
            enqueue_priority::min,
            [&mutex, &cvar_all_sync, &cvar_caller_return, &count_workers_not_sync, &counter_caller]() {
              std::unique_lock<std::mutex> lock(mutex);

              // Sync all workers. Only when all workers are in sync, we can carry on.
              count_workers_not_sync--;
              if (count_workers_not_sync > 0) {
                cvar_all_sync.wait(lock, [&count_workers_not_sync]() { return count_workers_not_sync == 0; });
              } else {
                cvar_all_sync.notify_all();
              }

              // When all workers passed the condition variable, we can notify the caller.
              if (--counter_caller == 0) {
                cvar_caller_return.notify_one();
              }
            })) {
      // Queue was deactivated.
      std::unique_lock<std::mutex> lock(mutex);
      count_workers_not_sync--;
    }
  }

  // Caller blocks waiting for all workers to sync.
  std::unique_lock<std::mutex> lock(mutex);
  cvar_caller_return.wait(lock, [&counter_caller]() { return counter_caller == 0; });
}

std::function<void()> priority_task_worker_pool::create_pop_loop_task()
{
  return [this]() {
    unique_task job;
    while (true) {
      if (not this->queue.pop_blocking(job)) {
        break;
      }
      job();
    }
  };
}

// ---- non-prioritized task worker pool

template <concurrent_queue_policy QueuePolicy>
task_worker_pool<QueuePolicy>::~task_worker_pool()
{
  stop();
}

template <concurrent_queue_policy QueuePolicy>
void task_worker_pool<QueuePolicy>::stop()
{
  DL_scheduler::getInstance().stop_flag.store(true);
  if (this->check_loop.joinable()) {
    this->check_loop.join();
  }
  for (unsigned i = 0; i < this->nof_workers(); i++) {
    is_yield[i] = true;
  }
  unsigned count = 0;
  for (unique_thread& w : worker_threads) {
    if (w.running()) {
      this->queue.request_stop();
      w.join();
      this->logger.info("Task worker \"{}\" finished.", worker_threads[count].get_name());
    }
    count++;
  }
}

template <concurrent_queue_policy QueuePolicy>
std::function<void()> task_worker_pool<QueuePolicy>::create_pop_loop_task()
{
  return [this]() {
    const int                    index = this->update_id();
    std::unique_lock<std::mutex> lck(*mtx[index - 1]);
    fmt::print("Pool name:{} - ID in pop loop task is {}, yield state is {}\n",
               this->pool_name,
               index,
               this->is_yield[index - 1]);
    unique_task job;
    while (true) {
      while (!is_yield[index - 1]) {
        cv[index - 1]->wait(lck);
      }

      if (not this->queue.pop_blocking(job)) {
        break;
      }
      if (check_poolname()) {
        auto now = std::chrono::system_clock::now();
        job.set_processing_time(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
      }
      job();
      if (check_poolname()) {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        job.set_end_processing_time(
            std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
        if (this->pool_name.find("up_phy_dl") != std::string::npos) {
          // std::lock_guard<std::mutex> lock(write_mutex);
          dl_logfile_stream << std::put_time(std::localtime(&t), "%Y-%m-%d %H.%M.%S") << " "
                            << "\ttask finished execution, "
                            << "\twait time is " << job.get_processing_time() - job.get_in_queue_time() << "us, "
                            << "\texecute time is " << job.get_end_processing_time() - job.get_processing_time()
                            << "us";
          if (logger.debug.enabled()) {
            dl_logfile_stream << ", \tpush_task time is " << job.get_in_queue_time() << ", \ttask finished time is "
                              << job.get_end_processing_time() << "\tqueue length when pushing tasks is "
                              << job.get_queue_length() << ", \tqueue length when finishing tasks is "
                              << this->nof_pending_tasks();
          }
          dl_logfile_stream << std::endl;
          dl_thread_state::getInstance().update_exec_time(job.get_end_processing_time() - job.get_processing_time());
          dl_thread_state::getInstance().update_wait_time(job.get_processing_time() - job.get_in_queue_time());
          dl_thread_state::getInstance().update_pop_time(job.get_processing_time());
          dl_thread_state::getInstance().update_length(this->nof_pending_tasks());
        } else {
          // std::lock_guard<std::mutex> lock(write_mutex);
          pusch_logfile_stream << std::put_time(std::localtime(&t), "%Y-%m-%d %H.%M.%S") << " "
                               << "\ttask finished execution, "
                               << "\twait time is " << job.get_processing_time() - job.get_in_queue_time() << "us, "
                               << "\texecute time is " << job.get_end_processing_time() - job.get_processing_time()
                               << "us";
          if (logger.debug.enabled()) {
            pusch_logfile_stream << ", \tpush_task time is " << job.get_in_queue_time() << ", \ttask finished time is "
                                 << job.get_end_processing_time() << "\tqueue length when pushing tasks is "
                                 << job.get_queue_length() << ", \tqueue length when finishing tasks is "
                                 << this->nof_pending_tasks();
          }
          pusch_logfile_stream << std::endl;
          pusch_thread_state::getInstance().update_exec_time(job.get_end_processing_time() - job.get_processing_time());
          pusch_thread_state::getInstance().update_wait_time(job.get_processing_time() - job.get_in_queue_time());
          pusch_thread_state::getInstance().update_pop_time(job.get_processing_time());
          pusch_thread_state::getInstance().update_length(this->nof_pending_tasks());
        }
      }
    }
  };
}

template <concurrent_queue_policy QueuePolicy>
void task_worker_pool<QueuePolicy>::wait_pending_tasks()
{
  std::mutex              mutex;
  std::condition_variable cvar_all_sync, cvar_caller_return;

  // Check if there are workers still running.
  unsigned workers_running = 0;
  for (unsigned i = 0; i != nof_workers(); ++i) {
    if (worker_threads[i].running()) {
      workers_running++;
    }
  }
  if (workers_running == 0) {
    // If no worker is running, return immediately.
    return;
  }

  // This will block all workers until all of them are running the enqueued task.
  unsigned count_workers_not_sync = workers_running;
  unsigned counter_caller         = workers_running;
  for (unsigned i = 0; i != workers_running; ++i) {
    if (not this->queue.push_blocking(
            [&mutex, &cvar_all_sync, &cvar_caller_return, &count_workers_not_sync, &counter_caller]() {
              std::unique_lock<std::mutex> lock(mutex);

              // Sync all workers. Only when all workers are in sync, we can carry on.
              count_workers_not_sync--;
              if (count_workers_not_sync > 0) {
                cvar_all_sync.wait(lock, [&count_workers_not_sync]() { return count_workers_not_sync == 0; });
              } else {
                cvar_all_sync.notify_all();
              }

              // When all workers passed the condition variable, we can notify the caller.
              if (--counter_caller == 0) {
                cvar_caller_return.notify_one();
              }
            })) {
      // Queue was deactivated.
      std::unique_lock<std::mutex> lock(mutex);
      count_workers_not_sync--;
    }
  }

  // Caller blocks waiting for all workers to sync.
  std::unique_lock<std::mutex> lock(mutex);
  cvar_caller_return.wait(lock, [&counter_caller]() { return counter_caller == 0; });
}

template <concurrent_queue_policy QueuePolicy>
void task_worker_pool<QueuePolicy>::thread_force_sleep(unsigned index)
{
  report_fatal_error_if_not(index < is_yield.size() && index >= 0,
                            "Index of threads must be smaller than number of workers and greater than 0");
  is_yield[index] = is_yield[index] & false;
}

template <concurrent_queue_policy QueuePolicy>
void task_worker_pool<QueuePolicy>::thread_force_wake(unsigned index)
{
  report_fatal_error_if_not(index < is_yield.size() && index >= 0,
                            "Index of threads must be smaller than number of workers and greater than 0");
  // std::unique_lock <std::mutex> lck(*(this->mtx[index]));
  is_yield[index] = is_yield[index] | true;
  this->cv[index]->notify_all();
}

// Explicit specializations of the task_worker_pool.
template class srsran::task_worker_pool<concurrent_queue_policy::lockfree_mpmc>;
template class srsran::task_worker_pool<concurrent_queue_policy::locking_mpmc>;
