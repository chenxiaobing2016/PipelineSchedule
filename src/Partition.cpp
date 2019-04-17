#include <Partition.h>
#include <iostream>

Partition::Partition(TaskGraph tg, Processor p)
    : tg_(tg), p_(p) {}

void Partition::run() {
  std::vector<unsigned> critical_path;
  getCriticalPath(critical_path, std::vector<unsigned>({tg_.entry}), 0, 0);
  std::cout << "critical_path size: " << critical_path.size() << std::endl;
  assert(critical_path.size() > 0);

  float profit = 0;
  unsigned task_id = -1, partition_nr = 1;

  if (critical_path.size() > 2) {
    tryPartition(critical_path, profit, partition_nr, task_id);
  }
  std::cout << "Partition nr: " << partition_nr << std::endl;
  if (partition_nr > 1) {
    taskPartition(partition_nr, task_id);
  }
}
#if 0
void Partition::run() {
  std::vector<std::vector<unsigned>> critical_path;
  getCriticalPath(critical_path, std::vector<unsigned>({tg_.entry}));
  std::cout << "critical_path size: " << critical_path.size() << std::endl;
  assert(critical_path.size() > 0);

  float profit = 0;
  unsigned task_id = -1, partition_nr = 1;
  for (unsigned path_idx = 0; path_idx < critical_path.size(); ++path_idx) {
    float tmp_profit = 0;
    unsigned tmp_partition_nr = 1, tmp_task_id = -1;
    if (critical_path[path_idx].size() > 2) {
      tryPartition(critical_path[path_idx], tmp_profit, tmp_partition_nr, tmp_task_id);
    }
    if (profit < tmp_profit) {
      profit = tmp_profit;
      task_id = tmp_task_id;
      partition_nr = tmp_partition_nr;
    }
  }
  if (partition_nr > 1) {
    taskPartition(partition_nr, task_id);
  }
}
#endif

void Partition::taskPartition(unsigned partition_nr, unsigned task_id) {
  TaskNode task = tg_.tasks[task_id];
  std::vector<TaskNode> rep_tasks = std::vector<TaskNode>(partition_nr, task);
  for (unsigned idx = 0; idx < partition_nr; ++idx) {
    rep_tasks[idx].in_size /= partition_nr;
    rep_tasks[idx].comp_size /= partition_nr;
    rep_tasks[idx].out_size /= partition_nr;
  }

  std::vector<TaskNode> tasks;
  for (unsigned idx = 0; idx < tg_.tasks.size() - 1; ++idx) {
    if (idx < task_id) {
      tasks.push_back(tg_.tasks[idx]);
    } else {
      tasks.push_back(tg_.tasks[idx + 1]);
    }
  }
  tasks.insert(tasks.end(), rep_tasks.begin(), rep_tasks.end());

  std::vector<std::vector<float>> comm_size = std::vector<std::vector<float>>(
    tg_.tasks.size() + partition_nr - 1, std::vector<float>(tg_.tasks.size() + partition_nr - 1, -1));
  for (unsigned src_idx = 0; src_idx < tg_.tasks.size() - 1; ++src_idx) {
    for (unsigned dst_idx = 0; dst_idx < tg_.tasks.size() - 1; ++dst_idx) {
      if (src_idx < task_id && dst_idx < task_id) {
        comm_size[src_idx][dst_idx] = tg_.comm_size[src_idx][dst_idx];
      } else if (src_idx < task_id && dst_idx >= task_id) {
        comm_size[src_idx][dst_idx] = tg_.comm_size[src_idx][dst_idx + 1];
      } else if (src_idx >= task_id && dst_idx < task_id) {
        comm_size[src_idx][dst_idx] = tg_.comm_size[src_idx + 1][dst_idx];
      } else if (src_idx >= task_id && dst_idx >= task_id) {
        comm_size[src_idx][dst_idx] = tg_.comm_size[src_idx + 1][dst_idx + 1];
      }
    }
  }
  for (unsigned pre_idx = 0; pre_idx < tg_.precedence[task_id].size(); ++pre_idx) {
    unsigned idx = tg_.precedence[task_id][pre_idx];
    for (int i = 0; i < partition_nr; ++i) {
      if (idx < task_id) {
        comm_size[idx][tg_.tasks.size() + i - 1] = tg_.comm_size[idx][task_id] / partition_nr;
      } else {
        comm_size[idx - 1][tg_.tasks.size() + i - 1] = tg_.comm_size[idx][task_id] / partition_nr;
      }
    }
  }
  for (unsigned succ_idx = 0; succ_idx < tg_.successor[task_id].size(); ++succ_idx) {
    unsigned idx = tg_.successor[task_id][succ_idx];
    for (int i = 0; i < partition_nr; ++i) {
      if (idx < task_id) {
        comm_size[tg_.tasks.size() + i - 1][idx] = tg_.comm_size[task_id][idx] / partition_nr;
      } else {
        comm_size[tg_.tasks.size() + i - 1][idx - 1] = tg_.comm_size[task_id][idx] / partition_nr;
      }
    }
  }
  tg_.tasks = tasks;
  tg_.comm_size = comm_size;
  // std::cout << "task id: " << task_id << std::endl;
  // std::cout << "----------" << std::endl;
  tg_.setTaskRelation();
  // std::cout << "++++++++++" << std::endl;
}

void Partition::tryPartition(std::vector<unsigned> critical_path, float& profit, unsigned& partition_nr, unsigned& task_id) {
  profit = 0;
  for (unsigned path_task_idx = 1; path_task_idx < critical_path.size() - 1; ++path_task_idx) {
    float start_time, finish_time;
    unsigned tmp_task_id = critical_path[path_task_idx];
    start_time  = tg_.tasks[tmp_task_id].start_time;
    finish_time = tg_.tasks[tmp_task_id].finish_time;
    // float pre_curr_comm_time = tg_.tasks[tmp_task_id].in_size / p_.bandwidth[tg_.tasks[critical_path[path_task_idx - 1]].fu_idx][tg_.tasks[tmp_task_id].fu_idx];
    // float execute_time = tg_.tasks[tmp_task_id].comp_size / p_.fu_info[tmp_task_id].speed;
    // float curr_succ_comm_time = tg_.tasks[critical_path[path_task_idx + 1]].in_size / p_.bandwidth[tg_.tasks[tmp_task_id].fu_idx][tg_.tasks[critical_path[path_task_idx + 1]].fu_idx];
    std::vector<unsigned> fus = getFreeFU(tg_.tasks[tmp_task_id].op.type, start_time, finish_time);
    std::vector<FU> fu_info = p_.fu_info;
    std::sort(fus.begin(), fus.end(), [&fu_info](unsigned a, unsigned b){ return fu_info[a].speed > fu_info[b].speed; });
    // std::cout << "min size: " << tg_.tasks[tmp_task_id].op.min_size << std::endl;
    // int max_partition_nr = (int)(tg_.tasks[tmp_task_id].in_size / tg_.tasks[tmp_task_id].op.min_size);
    int max_partition_nr = INT_MAX;
    // std::cout << "Max partition nr: " << max_partition_nr << std::endl;
    while (fus.size() > max_partition_nr) { fus.pop_back(); }
    float speed_sum = p_.fu_info[tg_.tasks[tmp_task_id].fu_idx].speed;
    for (unsigned fu_idx = 0; fu_idx < fus.size(); ++fu_idx) {
      speed_sum += p_.fu_info[fus[fu_idx]].speed;
    }
    float tmp_profit = finish_time - start_time - tg_.tasks[tmp_task_id].comp_size / speed_sum;
    if (tmp_profit > profit) {
      profit = tmp_profit;
      partition_nr = fus.size() + 1;
      task_id = tmp_task_id;
    }
  }
}

void Partition::getCriticalPath(std::vector<unsigned>& critical_path,
                                std::vector<unsigned> partial_path,
                                float critical_cost, float partial_cost) {
  if (partial_path.empty()) {
    partial_path.push_back(tg_.entry);
    partial_cost = 0;
  }
  if (partial_path.back() == tg_.exit) {
    if (partial_cost > critical_cost) {
      critical_path = partial_path;
      critical_cost = partial_cost;
    }
    return;
  }
  for (unsigned i = 0; i < tg_.successor[partial_path.back()].size(); ++i) {
    unsigned back_task_id = partial_path.back();
    unsigned curr_task_id = tg_.successor[back_task_id][i];
    TaskNode back_task = tg_.tasks[back_task_id];
    TaskNode curr_task = tg_.tasks[curr_task_id];
    float bandwidth = p_.bandwidth[tg_.tasks[back_task_id].fu_idx][tg_.tasks[curr_task_id].fu_idx];
    float comm_size = tg_.comm_size[back_task_id][curr_task_id];
    float comm_time = comm_size / bandwidth;
    float start_time = curr_task.start_time;
    float finish_time = curr_task.finish_time;
    partial_path.push_back(curr_task_id);
    partial_cost += (finish_time - start_time) + comm_time;
    getCriticalPath(critical_path, partial_path, critical_cost, partial_cost);
    partial_cost -= (finish_time - start_time) + comm_time;
    partial_path.pop_back();
  }
}

#if 0
void Partition::getCriticalPath(std::vector<std::vector<unsigned>>& critical_path,
                     std::vector<unsigned> partial_path) {
  if (partial_path.empty()) {
    partial_path.push_back(tg_.entry);
  }
  if (partial_path.back() == tg_.exit) {
    critical_path.push_back(partial_path);
    return;
  }
  for (unsigned i = 0; i < tg_.successor[partial_path.back()].size(); ++i) {
    unsigned back_task_id = partial_path.back();
    unsigned curr_task_id = tg_.successor[back_task_id][i];
    TaskNode back_task = tg_.tasks[back_task_id];
    TaskNode curr_task = tg_.tasks[curr_task_id];
    float back_finish_time = back_task.finish_time;
    float start_time = curr_task.start_time;
    float bandwidth = p_.bandwidth[tg_.tasks[back_task_id].fu_idx][tg_.tasks[curr_task_id].fu_idx];
    float comm_size = tg_.comm_size[back_task_id][curr_task_id];
    float comm_time = comm_size / bandwidth;
    if (std::abs(back_finish_time + comm_time - start_time) < 1e-6) {
      partial_path.push_back(curr_task_id);
      getCriticalPath(critical_path, partial_path);
      partial_path.pop_back();
    //  } else {
    //    std::cout << tg_.exit << " " <<back_task_id << " " << curr_task_id << " " << std::abs(back_finish_time + comm_time - start_time) << std::endl;
    }
  }
}
#endif

std::vector<unsigned> Partition::getFreeFU(OperationType op, float start, float finish) {
  std::vector<unsigned> ret;
  for (unsigned idx = 0; idx < p_.opt_fu_idx[op].size(); ++idx) {
    bool avail = true;
    unsigned fu_idx = p_.opt_fu_idx[op][idx];
    auto items = p_.fu_info[fu_idx].task_items;
    for (unsigned i = 0; i < items.size(); ++i) {
      if ((items[i].start_time < start && items[i].finish_time > start)
          || (items[i].start_time < finish && items[i].finish_time > finish)) {
        avail = false;
        break;
      }
    }
    if (avail) {
      ret.push_back(fu_idx);
    }
  }
  return ret;
}

TaskGraph Partition::getPartitionTaskGraph() {
  return tg_;
}

Processor Partition::getProcessor() {
  return p_;
}
