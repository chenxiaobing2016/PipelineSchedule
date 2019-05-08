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

  unsigned task_id = -1, partition_nr = 1;

  if (critical_path.size() > 2) {
    tryPartition(critical_path, partition_nr, task_id);
  }
  // std::cout << "Partition nr: " << partition_nr << std::endl;
  if (partition_nr > 1) {
    taskPartition(partition_nr, task_id);
  }
}

void showCommSize(std::vector<std::vector<float>> comm_size) {
  for (unsigned i = 0; i < comm_size.size(); ++i) {
    for (unsigned j = 0; j < comm_size.size(); ++j) {
      std::cout << std::setw(5) << comm_size[i][j] << " ";
    }
    std::cout << std::endl;
  }
}

void Partition::taskPartition(unsigned partition_nr, unsigned task_id) {
  if (partition_nr == 1) { return; }
  TaskNode task = tg_.tasks[task_id];
  std::vector<TaskNode> rep_tasks = std::vector<TaskNode>(partition_nr, task);

  for (unsigned idx = 0; idx < tg_.precedence[task_id].size(); ++idx) {
    unsigned pre_idx = tg_.precedence[task_id][idx];
    float inc_size = tg_.comm_size[pre_idx][task_id] / tg_.tasks[task_id].in_size * partition_nr * tg_.tasks[task_id].punish_io_size;
    tg_.comm_size[pre_idx][task_id] += inc_size;
    tg_.tasks[pre_idx].out_size += inc_size;
  }

  for (unsigned idx = 0; idx < tg_.successor[task_id].size(); ++idx) {
    unsigned succ_idx = tg_.successor[task_id][idx];
    float inc_size = tg_.comm_size[task_id][succ_idx] / tg_.tasks[task_id].out_size * partition_nr * tg_.tasks[task_id].punish_io_size;
    tg_.comm_size[task_id][succ_idx] += inc_size;
    tg_.tasks[succ_idx].out_size += inc_size;
  }
  tg_.tasks[task_id].in_size = partition_nr * tg_.tasks[task_id].punish_io_size;
  tg_.tasks[task_id].out_size = partition_nr * tg_.tasks[task_id].punish_io_size;
  tg_.tasks[task_id].comp_size = partition_nr * tg_.tasks[task_id].punish_comp_size;

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
  // TODO pre_comm_size += partition_nr * tg_.tasks[task_id].punish_io_size;
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

void Partition::tryPartition(std::vector<unsigned> critical_path, unsigned& partition_nr, unsigned& task_id) {
  float profit = 0;
  for (unsigned path_task_idx = 1; path_task_idx < critical_path.size() - 1; ++path_task_idx) {
    unsigned pre_task_id = critical_path[path_task_idx - 1];
    unsigned curr_task_id = critical_path[path_task_idx];
    unsigned post_task_id = critical_path[path_task_idx + 1];

    unsigned pre_fu_idx = tg_.tasks[pre_task_id].fu_idx;
    unsigned curr_fu_idx = tg_.tasks[curr_task_id].fu_idx;
    unsigned post_fu_idx = tg_.tasks[post_task_id].fu_idx;

    float start_time  = tg_.tasks[curr_task_id].start_time;
    float finish_time = tg_.tasks[curr_task_id].finish_time;
    std::vector<unsigned> fus = getFreeFU(tg_.tasks[curr_task_id].op.type, start_time, finish_time);
    fus.push_back(curr_fu_idx);

    std::vector<FU> fu_info = p_.fu_info;
    std::sort(fus.begin(), fus.end(), [&fu_info](unsigned a, unsigned b){ return fu_info[a].speed > fu_info[b].speed; });
    int max_partition_nr = INT_MAX;
    while (fus.size() > max_partition_nr) { fus.pop_back(); }

    float in_comm_size = tg_.comm_size[pre_task_id][curr_task_id];
    float out_comm_size = tg_.comm_size[curr_task_id][post_task_id];
    float comp_size = tg_.tasks[curr_task_id].comp_size;

    float in_comm_time = 0, out_comm_time = 0, comp_time = 0;
    float in_bandwidth = p_.bandwidth[pre_fu_idx][curr_fu_idx];
    float out_bandwidth = p_.bandwidth[curr_fu_idx][post_fu_idx];
    if (in_bandwidth > 0) {
      in_comm_time = in_comm_size / in_bandwidth;
    }
    if (out_bandwidth > 0) {
      out_comm_time = out_comm_size / out_bandwidth;
    }
    comp_time = comp_size / p_.fu_info[curr_fu_idx].speed;

    float p_in_comm_time = 0, p_out_comm_time = 0, p_comp_time = 0;
    unsigned nr = fus.size();
    for (unsigned i = 0; i < nr; ++i) {
      unsigned fu_idx = fus[i];
      if (p_.bandwidth[pre_fu_idx][fu_idx] > 0) {
        p_in_comm_time = std::max(p_in_comm_time, (in_comm_size / nr + tg_.tasks[curr_task_id].punish_io_size) / p_.bandwidth[pre_fu_idx][fu_idx]);
      }
      if (p_.bandwidth[fu_idx][post_fu_idx] > 0) {
        p_out_comm_time = std::max(p_out_comm_time, (out_comm_size / nr + tg_.tasks[curr_task_id].punish_io_size) / p_.bandwidth[fu_idx][post_fu_idx]);
      }
      p_comp_time = std::max(p_comp_time, (comp_size / nr + tg_.tasks[curr_task_id].punish_comp_size) / p_.fu_info[fu_idx].speed);
    }
    float tmp_profit = in_comm_time + comp_time + out_comm_time - (p_in_comm_time + p_comp_time + p_out_comm_time);
    if (tmp_profit > profit) {
      profit = tmp_profit;
      partition_nr = nr;
      task_id = curr_task_id;
    }
  }
}

void Partition::getCriticalPath(std::vector<unsigned> &critical_path) {
  std::vector<std::vector<unsigned>> succ = tg_.successor;
  std::vector<std::vector<unsigned>::iterator> pos;
  std::vector<std::vector<unsigned>::iterator> pos_end;

  auto& entry_succ = succ[tg_.entry];
  pos.push_back(entry_succ.begin());
  pos_end.push_back(entry_succ.end());
  while (true) {
    bool status = true;
    while (*(pos.back()) != tg_.exit) {
      assert(succ[*pos.back()].size() > 0);
      auto& succ_vec = succ[*(pos.back())];
      pos.push_back(succ_vec.begin());
      pos_end.push_back(succ_vec.end());
      unsigned curr_task_id = *(pos.back());
      unsigned pre_task_id;
      if (pos.size() == 1) {
        pre_task_id = tg_.entry;
      } else {
        pre_task_id = **(std::prev(pos.end(), 2));
      }
      float start_time = tg_.tasks[curr_task_id].start_time;
      float pre_end_time = tg_.tasks[pre_task_id].finish_time;

      float comm_time = 0;
      float comm_size = tg_.comm_size[pre_task_id][curr_task_id];
      float bandwidth = p_.bandwidth[tg_.tasks[pre_task_id].fu_idx][tg_.tasks[curr_task_id].fu_idx];
      if (bandwidth > 0) {
        comm_time = comm_size / bandwidth;
      }
      if (std::abs(pre_end_time + comm_time - start_time) > 1e-4) {
        status = false;
        break;
      }
    }
    if (status) {
      critical_path.clear();
      for (auto i : pos) {
        critical_path.push_back(*i);
      }
      return;
    }
    while (std::next(pos.back()) == pos_end.back()) {
      pos.pop_back();
      pos_end.pop_back();
    }
    if (pos.empty()) {
      break;
    } else {
      ++pos.back();
    }
  }

  // }
  //
  // void Partition::getCriticalPath(std::vector<unsigned> &critical_path) {

  float max_cost = FLT_MAX;
  succ = tg_.successor;
  pos.clear();
  pos_end.clear();
  critical_path.clear();
  std::vector<float> step_cost;

  entry_succ = succ[tg_.entry];
  pos.push_back(entry_succ.begin());
  pos_end.push_back(entry_succ.end());
  float curr_path_cost = 0;
  while (true) {
    // get next path
    while (*(pos.back()) != tg_.exit) {
      assert(succ[*pos.back()].size() > 0);
      auto& succ_vec = succ[*(pos.back())];
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

std::vector<unsigned> Partition::getFreeFU(OperationType op, float start, float finish) {
  std::vector<unsigned> ret;
  for (unsigned idx = 0; idx < p_.opt_fu_idx[op].size(); ++idx) {
    bool avail = true;
    unsigned fu_idx = p_.opt_fu_idx[op][idx];
    auto items = p_.fu_info[fu_idx].task_items;
    for (unsigned i = 0; i < items.size(); ++i) {

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
