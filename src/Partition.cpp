#include <Partition.h>
#include <iostream>
#include <iomanip>

Partition::Partition(TaskGraph tg, Processor p)
    : tg_(tg), p_(p) {}

void Partition::run() {
  std::vector<unsigned> critical_path;
  // getCriticalPath(critical_path, std::vector<unsigned>({tg_.entry}), 0, 0);
  getCriticalPath(critical_path);
  // std::cout << "critical_path size: " << critical_path.size() << std::endl;
  assert(critical_path.size() > 0);

  float profit = 0;
  unsigned task_id = -1, partition_nr = 1;

  if (critical_path.size() > 2) {
    tryPartition(critical_path, profit, partition_nr, task_id);
  }
  // std::cout << "Partition nr: " << partition_nr << std::endl;
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
void showCommSize(std::vector<std::vector<float>> comm_size) {
  for (unsigned i = 0; i < comm_size.size(); ++i) {
    for (unsigned j = 0; j < comm_size.size(); ++j) {
      std::cout << std::setw(5) << comm_size[i][j] << " ";
    }
    std::cout << std::endl;
  }
}

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

  float pre_comm_size = 0;
  for (unsigned pre_idx = 0; pre_idx < tg_.precedence[task_id].size(); ++pre_idx) {
    pre_comm_size += tg_.comm_size[tg_.precedence[task_id][pre_idx]][task_id];
  }
  pre_comm_size /= partition_nr;
  unsigned pre_idx = 0;
  float pre_remin_size = tg_.comm_size[tg_.precedence[task_id][pre_idx]][task_id];
  for (int i = 0; i < partition_nr; ++i) {
    float in_size = pre_comm_size;
    if (i == partition_nr - 1) {
      if (tg_.precedence[task_id][pre_idx] > task_id) {
        comm_size[tg_.precedence[task_id][pre_idx] - 1][tg_.tasks.size() + i - 1] = pre_remin_size;
      } else {
        comm_size[tg_.precedence[task_id][pre_idx]][tg_.tasks.size() + i - 1] = pre_remin_size;
      }
      for (++pre_idx; pre_idx < tg_.precedence[task_id].size(); ++pre_idx) {
        if (tg_.precedence[task_id][pre_idx] > task_id) {
          comm_size[tg_.precedence[task_id][pre_idx] - 1][tg_.tasks.size() + i - 1] = tg_.comm_size[tg_.precedence[task_id][pre_idx]][task_id];
        } else {
          comm_size[tg_.precedence[task_id][pre_idx]][tg_.tasks.size() + i - 1] = tg_.comm_size[tg_.precedence[task_id][pre_idx]][task_id];
        }
      }
      break;
    }
    while (in_size >= pre_remin_size && in_size > 1e-6) {
      if (tg_.precedence[task_id][pre_idx] > task_id) {
        comm_size[tg_.precedence[task_id][pre_idx] - 1][tg_.tasks.size() + i - 1] = pre_remin_size;
      } else {
        comm_size[tg_.precedence[task_id][pre_idx]][tg_.tasks.size() + i - 1] = pre_remin_size;
      }
      in_size -= pre_remin_size;
      pre_idx += 1;
      if (pre_idx < tg_.precedence[task_id].size()) {
        pre_remin_size = tg_.comm_size[tg_.precedence[task_id][pre_idx]][task_id];
      }
    }
    if (in_size < pre_remin_size && in_size > 1e-6) {
      if (tg_.precedence[task_id][pre_idx] > task_id) {
        comm_size[tg_.precedence[task_id][pre_idx] - 1][tg_.tasks.size() + i - 1] = in_size;
      } else {
        comm_size[tg_.precedence[task_id][pre_idx]][tg_.tasks.size() + i - 1] = in_size;
      }
      pre_remin_size -= in_size;
    }
  }
#if 0
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
#endif

  float succ_comm_size = 0;
  for (unsigned succ_idx = 0; succ_idx < tg_.successor[task_id].size(); ++succ_idx) {
    succ_comm_size += tg_.comm_size[task_id][tg_.successor[task_id][succ_idx]];
  }
  succ_comm_size /= partition_nr;
  unsigned succ_idx = 0;
  float succ_remin_size = tg_.comm_size[task_id][tg_.successor[task_id][succ_idx]];
  for (int i = 0; i < partition_nr; ++i) {
    float out_size = succ_comm_size;
    if (i == partition_nr - 1) {
      if (tg_.successor[task_id][succ_idx] > task_id) {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx] - 1] = succ_remin_size;
      } else {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx]] = succ_remin_size;
      }
      for (++succ_idx; succ_idx < tg_.successor[task_id].size(); ++succ_idx) {
        if (tg_.successor[task_id][succ_idx] > task_id) {
          comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx] - 1] = tg_.comm_size[task_id][tg_.successor[task_id][succ_idx]];
        } else {
          comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx]] = tg_.comm_size[task_id][tg_.successor[task_id][succ_idx]];
        }
      }
      break;
    }
    while (out_size >= succ_remin_size && out_size > 1e-6) {
      if (tg_.successor[task_id][succ_idx] > task_id) {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx] - 1] = succ_remin_size;
      } else {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx]] = succ_remin_size;
      }
      out_size -= succ_remin_size;
      succ_idx += 1;
      if (succ_idx < tg_.successor[task_id].size()) {
        // succ_remin_size = comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx]];
        succ_remin_size = tg_.comm_size[task_id][tg_.successor[task_id][succ_idx]];
      }
    }
    if (out_size < succ_remin_size && out_size > 1e-6) {
      if (tg_.successor[task_id][succ_idx] > task_id) {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx] - 1] = out_size;
      } else {
        comm_size[tg_.tasks.size() + i - 1][tg_.successor[task_id][succ_idx]] = out_size;
      }
      succ_remin_size -= out_size;
    }
  }
#if 0
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
#endif
  tg_.tasks = tasks;
  tg_.comm_size = comm_size;
  tg_.setTaskRelation();
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
    // std::cout << "Free fu number: " << fus.size() << std::endl;
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

void Partition::getCriticalPath(std::vector<unsigned> &critical_path) {
  float max_cost = FLT_MAX;
  std::vector<std::vector<unsigned>> succ = tg_.successor;
  std::vector<std::vector<unsigned>::iterator> pos;
  std::vector<std::vector<unsigned>::iterator> pos_end;
  std::vector<float> step_cost;

  auto& entry_succ = succ[tg_.entry];
  // pos.push_back(succ[tg_.entry].begin());
  // pos_end.push_back(succ[tg_.entry].end());
  pos.push_back(entry_succ.begin());
  pos_end.push_back(entry_succ.end());
  float curr_path_cost = 0;
  while (true) {
    // get next path
    while (*(pos.back()) != tg_.exit) {
      assert(succ[*pos.back()].size() > 0);
      auto& succ_vec = succ[*(pos.back())];
      // pos.push_back(succ[*(pos.back())].begin());
      // pos_end.push_back(succ[*(pos.back())].end());
      pos.push_back(succ_vec.begin());
      pos_end.push_back(succ_vec.end());
      float tmp_step_cost = tg_.tasks[*(pos.back())].comp_size / p_.fu_info[tg_.tasks[*(pos.back())].fu_idx].speed;
      if (pos.size() == 1) {
        tmp_step_cost += tg_.comm_size[tg_.entry][*(pos.back())] / p_.bandwidth[tg_.tasks[tg_.entry].fu_idx][tg_.tasks[*(pos.back())].fu_idx];
      } else {
        tmp_step_cost += tg_.comm_size[*(pos[pos.size() - 2])][*(pos.back())] / p_.bandwidth[tg_.tasks[*(pos[pos.size() - 2])].fu_idx][tg_.tasks[*(pos.back())].fu_idx];
      }
      curr_path_cost += tmp_step_cost;
      step_cost.push_back(tmp_step_cost);
      if (curr_path_cost > max_cost) {
        break;
      }
    }
    if (*(pos.back()) == tg_.exit && curr_path_cost < max_cost) {
      critical_path.clear();
      for (auto i : pos) {
        critical_path.push_back(*i);
      }
      max_cost = curr_path_cost;
    }
    while (std::next(pos.back()) == pos_end.back()) {
      curr_path_cost -= step_cost.back();
      pos.pop_back();
      pos_end.pop_back();
      step_cost.pop_back();
    }

    if (pos.empty()) {
      break;
    } else {
      ++pos.back();
      // reset the last step cost
      float tmp_step_cost = tg_.tasks[*(pos.back())].comp_size / p_.fu_info[tg_.tasks[*(pos.back())].fu_idx].speed;
      if (pos.size() == 1) {
        tmp_step_cost += tg_.comm_size[tg_.entry][*(pos.back())] / p_.bandwidth[tg_.tasks[tg_.entry].fu_idx][tg_.tasks[*(pos.back())].fu_idx];
      } else {
        tmp_step_cost += tg_.comm_size[*(pos[pos.size() - 2])][*(pos.back())] / p_.bandwidth[tg_.tasks[*(pos[pos.size() - 2])].fu_idx][tg_.tasks[*(pos.back())].fu_idx];
      }
      curr_path_cost = curr_path_cost - step_cost.back() + tmp_step_cost;
      step_cost.back() = tmp_step_cost;
    }
  }
}

#if 0
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
#endif

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
#if 0
      std::cout << "start: " << start << " finish: " << finish
                << " start time: " << items[i].start_time
                << " finish time: " << items[i].finish_time
                << " result: " <<
                               ((items[i].start_time < start && items[i].finish_time > start)
          || (items[i].start_time < finish && items[i].finish_time > finish)) << std::endl;
#endif

      if ((items[i].start_time - 1e-6 < start && items[i].finish_time + 1e-6 > start)
          || (items[i].start_time - 1e-6 < finish && items[i].finish_time + 1e-6 > finish)) {
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
