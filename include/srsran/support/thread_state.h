#ifndef THREAD_STATE_H
#define THREAD_STATE_H

#include <vector>

class dl_thread_state
{
public:
  static dl_thread_state& getInstance()
  {
    static dl_thread_state instance;
    return instance;
  }

  long getBlockTime(int index)
  {
    if (index < static_cast<int>(task_block_times.size())) {
      return task_block_times[index][(block_lens[index] + 99) % 100];
    } else {
      return -1; // If index is out of range, return -1
    }
  }

  void update_exec_time(long t)
  {
    exec_sum -= exec_time[exec_len];
    exec_time[exec_len] = t;
    exec_len            = (exec_len + 1) % 100;
    exec_sum += t;
  }

  void update_wait_time(long t)
  {
    wait_sum -= wait_time[wait_len];
    wait_time[wait_len] = t;
    wait_len            = (wait_len + 1) % 100;
    wait_sum += t;
  }

  void update_pop_time(long t)
  {
    if (prev != 0) {
      interval_sum -= pop_time_interval[exec_len];
      pop_time_interval[interval_len] = t - prev;
      interval_len                    = (interval_len + 1) % 100;
      interval_sum += t;
    }
    prev = t;
  }

  void update_length(long len)
  {
    if (task_len_queue[task_len] < len) {
      len_incre++;
    } else if (len_incre > 0) {
      len_incre--;
    }
    task_len_queue[task_len] = len;
    task_len                 = (task_len + 1) % 100;
  }

  void update_task_time(long t1, long t2)
  {
    create_time[create_len][0] = t1;
    create_time[create_len][1] = t2;
    create_len                 = (create_len + 1) % 200;
  }
  void update_task_block_time(int index, long t)
  {
    if (index >= static_cast<int>(task_block_times.size())) {
      task_block_times.resize(index + 1, std::vector<long>(100, 0));
      block_lens.resize(index + 1, 0);
    }
    task_block_times[index][block_lens[index]] = t;
    block_lens[index]                          = (block_lens[index] + 1) % 100;
  }

  std::vector<long> exec_time = std::vector<long>(100, 0);
  int               exec_len  = 0;
  long              exec_sum  = 0;

  std::vector<long> wait_time = std::vector<long>(100, 0);
  int               wait_len  = 0;
  long              wait_sum  = 0;

  std::vector<long> pop_time_interval = std::vector<long>(100, 0);
  int               interval_len      = 0;
  long              interval_sum      = 0;
  long              prev              = 0;

  std::vector<long> task_len_queue = std::vector<long>(100, 0);
  int               task_len       = 0;
  long              len_sum        = 0;
  int               len_incre      = 0;

  // 用于记录 worker 的阻塞时长
  std::vector<std::vector<long>> task_block_times = std::vector<std::vector<long>>(8, std::vector<long>(100, 0));
  std::vector<int>               block_lens       = std::vector<int>(8, 0);

  std::vector<std::vector<long>> create_time = std::vector<std::vector<long>>(200, std::vector<long>(2, 0));
  int                            create_len  = 0;
  std::vector<std::vector<long>> finish_time = std::vector<std::vector<long>>(200, std::vector<long>(2, 0));
  int                            finish_len  = 0;
};

class pusch_thread_state
{
public:
  static pusch_thread_state& getInstance()
  {
    static pusch_thread_state instance;
    return instance;
  }

  long getBlockTime(int index)
  {
    if (index < static_cast<int>(task_block_times.size())) {
      return task_block_times[index][(block_lens[index] + 99) % 100];
    } else {
      return -1; // If index is out of range, return -1
    }
  }

  void update_exec_time(long t)
  {
    exec_sum -= exec_time[exec_len];
    exec_time[exec_len] = t;
    exec_len            = (exec_len + 1) % 100;
    exec_sum += t;
  }

  void update_wait_time(long t)
  {
    wait_sum -= wait_time[wait_len];
    wait_time[wait_len] = t;
    wait_len            = (wait_len + 1) % 100;
    wait_sum += t;
  }

  void update_pop_time(long t)
  {
    if (prev != 0) {
      interval_sum -= pop_time_interval[exec_len];
      pop_time_interval[interval_len] = t - prev;
      interval_len                    = (interval_len + 1) % 100;
      interval_sum += t;
    }
    prev = t;
  }

  void update_length(long len)
  {
    if (task_len_queue[task_len] < len) {
      len_incre++;
    } else if (len_incre > 0) {
      len_incre--;
    }
    task_len_queue[task_len] = len;
    task_len                 = (task_len + 1) % 100;
  }

  void update_task_time(long t1, long t2)
  {
    create_time[create_len][0] = t1;
    create_time[create_len][1] = t2;
    create_len                 = (create_len + 1) % 100;
  }

  void update_task_block_time(int index, long t)
  {
    if (index >= static_cast<int>(task_block_times.size())) {
      task_block_times.resize(index + 1, std::vector<long>(100, 0));
      block_lens.resize(index + 1, 0);
    }
    task_block_times[index][block_lens[index]] = t;
    block_lens[index]                          = (block_lens[index] + 1) % 100;
  }

  std::vector<long> exec_time = std::vector<long>(100, 0);
  int               exec_len  = 0;
  long              exec_sum  = 0;

  std::vector<long> wait_time = std::vector<long>(100, 0);
  int               wait_len  = 0;
  long              wait_sum  = 0;

  std::vector<long> pop_time_interval = std::vector<long>(100, 0);
  int               interval_len      = 0;
  long              interval_sum      = 0;
  long              prev              = 0;

  std::vector<long> task_len_queue = std::vector<long>(100, 0);
  int               task_len       = 0;
  long              len_sum        = 0;
  int               len_incre      = 0;

  // 用于记录 worker 的阻塞时长
  std::vector<std::vector<long>> task_block_times = std::vector<std::vector<long>>(8, std::vector<long>(100, 0));
  std::vector<int>               block_lens       = std::vector<int>(8, 0);

  std::vector<std::vector<long>> create_time = std::vector<std::vector<long>>(100, std::vector<long>(2, 0));
  int                            create_len  = 0;
  std::vector<std::vector<long>> finish_time = std::vector<std::vector<long>>(100, std::vector<long>(2, 0));
  int                            finish_len  = 0;
};
#endif
