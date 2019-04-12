#include <algorithm>
#include <cfloat>
#include <climits>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Scheduler.h"

Scheduler::Scheduler(TaskGraph tg, Processor p)
    : tg_(tg), p_(p), l_(0) {}

void Scheduler::runHEFT() {
  reckonAvgCompCost();
  reckonAvgCommCost();
  reckonUpwardRank();
  sortByUpwardRank();
  scheduleHEFT();
}

void Scheduler::runCPOP() {
    reckonAvgCompCost();
    reckonAvgCommCost();
    reckonUpwardRank();
    reckonDownwardRank();
    sortByUpAndDownwardRank();
    scheduleCPOP();
}
void Scheduler::dumpScheduleResult(const std::string &file_name) {
  std::ofstream fout(file_name.c_str());
  for (unsigned p_idx = 0u; p_idx < p_.fu_info.size(); ++p_idx) {
    std::vector<TaskItem> task_items = p_.fu_info[p_idx].task_items;
    for (unsigned t_idx = 0u; t_idx < task_items.size(); ++t_idx) {
      TaskItem ti = task_items[t_idx];
      fout << ti.task_idx << " " << ti.start_time << " " << ti.finish_time << " ";
    }
    fout << std::endl;
  }
}

void Scheduler::dumpTaskGraph(const std::string &file_name) {
  std::ofstream fout(file_name.c_str());
  for (unsigned task_idx = 0u; task_idx < tg_.tasks.size(); ++task_idx) {
    std::string operation_type = toString(tg_.tasks[task_idx].op.type);
    int fu_id        = tg_.tasks[task_idx].fu_idx;
    float size       = tg_.tasks[task_idx].comp_size;
    float speed      = p_.fu_info[fu_id].speed;
    float start_time = tg_.tasks[task_idx].start_time;
    float end_time   = tg_.tasks[task_idx].finish_time;
    fout << task_idx       << " "
         << operation_type << " "
         << fu_id          << " "
         << size           << " "
         << speed          << " "
         << start_time     << " "
         << end_time       << " ";
    for (unsigned succ_idx = 0u; succ_idx < tg_.successor[task_idx].size(); ++succ_idx) {
      int succ_id    = tg_.successor[task_idx][succ_idx];
      float size     = tg_.comm_size[task_idx][succ_id];
      int succ_fu_id = tg_.tasks[succ_id].fu_idx;
      float speed    = p_.bandwidth[fu_id][succ_fu_id];
      float time     = size / speed;
      fout << succ_id << " "
           << size    << " "
           << speed   << " "
           << time    << " ";
    }
    fout << std::endl;
  }
}

void Scheduler::reckonAvgCompCost() {
  auto tasks = tg_.tasks;
  unsigned task_nr = tasks.size();

  // reckon average computational cost for each task.
  avg_comp_cost_.resize(task_nr, 0);
  for (unsigned task_idx = 0u; task_idx < task_nr; ++task_idx) {
    auto task = tasks[task_idx];
    OperationType opt = task.op.type;
    avg_comp_cost_[task_idx] = tasks[task_idx].comp_size / p_.op2avg_comp_speed[opt];
  }
}

void Scheduler::reckonAvgCommCost() {
  auto tasks = tg_.tasks;
  unsigned task_nr = tasks.size();

  avg_comm_cost_.resize(task_nr, std::vector<float>(task_nr, 0));
  for (unsigned src_idx = 0; src_idx < task_nr; ++src_idx) {
    for (unsigned dst_idx = 0; dst_idx < task_nr; ++dst_idx) {
      if  (tg_.comm_size[src_idx][dst_idx] == -1) {
        avg_comm_cost_[src_idx][dst_idx] = -1;
      } else if (src_idx != dst_idx) {
        auto src_opt = tasks[src_idx].op.type;
        auto dst_opt = tasks[dst_idx].op.type;
        float avg_bd = p_.op2op_avg_comm_speed[std::make_pair(src_opt, dst_opt)];
        avg_comm_cost_[src_idx][dst_idx] = l_ + tg_.comm_size[src_idx][dst_idx] /avg_bd;
      }
    }
  }
}

void Scheduler::reckonDownwardRank() {
    auto tasks = tg_.tasks;
    auto task_nr = tasks.size();
    downward_rank_.resize(task_nr, -1);
    std::unordered_set<unsigned> reckoned_tasks;
    while (reckoned_tasks.size() != task_nr) {
        for (auto idx = 0; idx < task_nr; ++idx) {
            if (reckoned_tasks.find(idx) != reckoned_tasks.end()) {
                continue;
            }
            bool is_ready = true;
            for (auto prec_idx : tg_.precedence[idx]) {
                if (downward_rank_[prec_idx] == -1) {
                    is_ready = false;
                    break;
                }
            }
            if (!is_ready) {
                continue;
            }
            for (auto prec_idx : tg_.precedence[idx]) {
                float comm_speed = p_.op2op_avg_comm_speed[std::make_pair(tasks[prec_idx].op.type,
                                                                          tasks[idx].op.type)];
                float tmp = tg_.comm_size[prec_idx][idx] / comm_speed
                        + tasks[prec_idx].comp_size / p_.op2avg_comp_speed[tasks[prec_idx].op.type]
                        + downward_rank_[prec_idx];
                downward_rank_[idx] = std::max(downward_rank_[idx], tmp);
            }
            reckoned_tasks.insert(idx);
        }
    }
}
void Scheduler::reckonUpwardRank() {
  auto tasks = tg_.tasks;
  auto task_nr = tasks.size();
  upward_rank_.resize(task_nr, -1);
  std::unordered_set<unsigned> reckoned_tasks;
  while (reckoned_tasks.size() != task_nr) {
    for (auto idx = 0; idx < task_nr; ++idx) {
      if (reckoned_tasks.find(idx) != reckoned_tasks.end()) {
        continue;
      }
      bool is_ready = true;
      for (auto succ_idx : tg_.successor[idx]) {
        if (upward_rank_[succ_idx] == -1) {
          is_ready = false;
          break;
        }
      }
      if (!is_ready) {
        continue;
      }
      upward_rank_[idx] = 0;
      for (auto succ_idx : tg_.successor[idx]) {
        float comm_speed = p_.op2op_avg_comm_speed[std::make_pair(tasks[idx].op.type,
                                                   tasks[succ_idx].op.type)];
        float tmp = tg_.comm_size[idx][succ_idx] / comm_speed + upward_rank_[succ_idx];
        upward_rank_[idx] = std::max(upward_rank_[idx], tmp);
      }
      upward_rank_[idx] += tasks[idx].comp_size / p_.op2avg_comp_speed[tasks[idx].op.type];
      reckoned_tasks.insert(idx);
    }
  }
}

void Scheduler::sortByUpwardRank() {
  auto tasks = tg_.tasks;
  unsigned task_nr = tasks.size();
  HEFT_sorted_task_idx_.resize(task_nr, 0);
  for (unsigned i = 0; i < task_nr; ++i) {
    HEFT_sorted_task_idx_[i] = i;
  }
  auto tmp_upward_rank = upward_rank_;
  auto cmp = [&tmp_upward_rank](unsigned a, unsigned b) {
    return tmp_upward_rank[a] >= tmp_upward_rank[b];
  };
  std::sort(HEFT_sorted_task_idx_.begin(), HEFT_sorted_task_idx_.end(), cmp);
}

void Scheduler::sortByUpAndDownwardRank() {
    auto tasks = tg_.tasks;
    unsigned task_nr = tasks.size();
    CPOP_sorted_task_idx_.resize(task_nr, 0);
    auto tmp_up_plus_down_rank = upward_rank_;
    for (unsigned i = 0; i < task_nr; ++i) {
        CPOP_sorted_task_idx_[i] = i;
        tmp_up_plus_down_rank[i] += downward_rank_[i];
    }
    auto cmp = [&tmp_up_plus_down_rank](unsigned a, unsigned b) {
        return tmp_up_plus_down_rank[a] >= tmp_up_plus_down_rank[b];
    };
    std::sort(CPOP_sorted_task_idx_.begin(), CPOP_sorted_task_idx_.end(), cmp);
}

void Scheduler::scheduleHEFT() {
  auto& tasks = tg_.tasks;
  unsigned task_nr = tasks.size();

  std::vector<std::vector<unsigned>> fu2task_idx(p_.fu_info.size(), std::vector<unsigned>());
  for (unsigned task_idx = 0; task_idx < task_nr; ++task_idx) {
    auto task = tasks[HEFT_sorted_task_idx_[task_idx]];
    auto opt = task.op.type;
    std::vector<unsigned> fu_condidates = p_.opt_fu_idx[opt];

    unsigned fu_id = -1;
    float start_time = FLT_MAX, finish_time = FLT_MAX;

    for (auto fu_idx : fu_condidates) {
      // reckon avail_time
      float avail_time = 0;
      float cpt_time = task.comp_size / p_.fu_info[fu_idx].speed;
      // [0 ~ no_dep_task_idx] is the ancestor of current task
      unsigned no_dep_task_idx = 0;
      std::vector<TaskItem> task_items = p_.fu_info[fu_idx].task_items;
      if (task_items.size() == 0) {
        avail_time = 0;
      } else {
        for (unsigned i = 0; i < task_items.size(); ++i) {
          if (tg_.isAncestor(task_items[i].task_idx, HEFT_sorted_task_idx_[task_idx])) {
            no_dep_task_idx = i + 1;
          }
        }
        for (unsigned i = no_dep_task_idx; i <= task_items.size(); ++i) {
          // i == 0 enter the second body.
          if (i == 0 && task_items[0].start_time >= cpt_time) {
            avail_time = 0;
            break;
          } else if (i > 0 && task_items[i].start_time - task_items[i - 1].finish_time >= cpt_time) {
            avail_time = task_items[i - 1].finish_time;
            break;
          } else if (i == task_items.size()) {
            avail_time = task_items[i - 1].finish_time;
            break;
          }
        }
      }

      float curr_start_time = avail_time;
      // select the max of {avail_time(fu), precedence(j) + comm_time(j, i)} as start time.
      for (unsigned pre_task_idx : tg_.precedence[HEFT_sorted_task_idx_[task_idx]]) {
        unsigned pre_fu_idx = tasks[pre_task_idx].fu_idx;
        float tmp_start_time = tasks[pre_task_idx].finish_time
                               + tg_.comm_size[pre_task_idx][HEFT_sorted_task_idx_[task_idx]]
                               / p_.bandwidth[pre_fu_idx][fu_idx];
        curr_start_time = std::max(tmp_start_time, curr_start_time);
      }
      float curr_finish_time = curr_start_time + cpt_time;
      if (finish_time > curr_finish_time) {
        fu_id = fu_idx;
        start_time = curr_start_time;
        finish_time = curr_finish_time;
      }
    }  // for fu_idx

    // set task group and processor status.
    tg_.tasks[HEFT_sorted_task_idx_[task_idx]].setFuInfo(fu_id, start_time, finish_time);
    TaskItem ti { HEFT_sorted_task_idx_[task_idx], start_time, finish_time};
    p_.fu_info[fu_id].insertTaskItem(ti);
  }
}

void Scheduler::scheduleCPOP() {
    std::vector<TaskNode> CPSet;
    CPSet.push_back(tg_.tasks.front());
    unsigned cur_idx = 0;
    while (CPSet.back().op.type != OUTPUT) {
        unsigned next_idx = -1;
        auto cur_sucs = tg_.successor[cur_idx];
        for (auto task_idx : CPOP_sorted_task_idx_) {
            auto next_iter = find(cur_sucs.begin(), cur_sucs.end(), task_idx);
            if (next_iter != cur_sucs.end()) {
                next_idx = *next_iter;
                break;
            }
        }
        CPSet.push_back(tg_.tasks[next_idx]);
        cur_idx = next_idx;
    }

    // schedule CPSet tasks
    std::vector<std::vector<float>> DP_table;
    std::vector<std::vector<unsigned>> DP_trace;
    for (auto CP_idx = 0; CP_idx < CPSet.size(); CP_idx++) {
        std::vector<float> DP_table_col;
        std::vector<unsigned> DP_trace_col;
        auto cur_task = CPSet[CP_idx];
        for (auto fu_idx : p_.opt_fu_idx[CPSet[CP_idx].op.type]) {
            float comp_time = cur_task.comp_size / p_.fu_info[fu_idx].speed;
            if (CP_idx == 0) {
                DP_table_col.push_back(comp_time);
                DP_table_col.push_back(0);
            } else {
                unsigned min_pre_idx = 0;
                float min_time = FLT_MAX;
                for (auto pre_idx = 0; pre_idx < DP_table.back().size(); pre_idx++) {
                    unsigned pre_fu_idx = p_.opt_fu_idx[CPSet[CP_idx - 1].op.type][pre_idx];
                    float comm_time = cur_task.out_size / p_.bandwidth[pre_fu_idx][fu_idx];
                    if (comp_time + comm_time < min_time) {
                        min_pre_idx = pre_idx;
                        min_time = comp_time + comm_time;
                    }
                }
                DP_table_col.push_back(min_time);
                DP_trace_col.push_back(min_pre_idx);
            }
        }
        DP_table.push_back(DP_table_col);
        DP_trace.push_back(DP_trace_col);
    }

    unsigned pre_idx = 0;
    for (auto task_idx = CPSet.size() - 1; task_idx > 0; task_idx--) {
        auto cur_type = CPSet[task_idx].op.type;
        auto pre_type = CPSet[task_idx - 1].op.type;
        if (task_idx == CPSet.size() - 1) {
            auto last_idx = std::max_element(DP_table[task_idx].begin(), DP_table[task_idx].end())
                    - DP_table[task_idx].end();
            pre_idx = DP_trace[task_idx][last_idx];
            unsigned fu_idx = p_.opt_fu_idx[cur_type][last_idx];
            unsigned pre_fu_idx = p_.opt_fu_idx[pre_type][pre_idx];
            CPSet[task_idx].fu_idx = fu_idx;
            CPSet[task_idx - 1].fu_idx = pre_fu_idx;
        } else {
            pre_idx = DP_trace[task_idx][pre_idx];
            unsigned pre_fu_idx = p_.opt_fu_idx[pre_type][pre_idx];
            CPSet[task_idx - 1].fu_idx = pre_fu_idx;
        }
    }
}
