#include <algorithm>
#include <cfloat>
#include <climits>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

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

TaskGraph Scheduler::getScheduledTaskGraph() {
  return tg_;
}

Processor Scheduler::getScheduledProcessor() {
  return  p_;
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
            downward_rank_[idx] = 0;
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
    return tmp_upward_rank[a] > tmp_upward_rank[b];
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
        return tmp_up_plus_down_rank[a] > tmp_up_plus_down_rank[b];
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
        // for (unsigned i = 0; i < task_items.size(); ++i) {
        //   if (tg_.isAncestor(task_items[i].task_idx, HEFT_sorted_task_idx_[task_idx])) {
        //     no_dep_task_idx = i + 1;
        //   }
        // }
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
void Scheduler::setCPsetMinFu(std::vector<unsigned> CPset) {
    std::unordered_map<OperationType, float> op_comp_size;
    std::unordered_map<std::pair<OperationType, OperationType>, float> op2op_comm_size;
    for (unsigned cp_idx = 0u; cp_idx < CPset.size(); ++cp_idx) {
        auto task_idx = CPset[cp_idx];
        OperationType opt = tg_.tasks[task_idx].op.type;
        float size = tg_.tasks[task_idx].comp_size;
        if (op_comp_size.count(opt) == 0) {
            op_comp_size[opt] = size;
        } else {
            op_comp_size[opt] += size;
        }
    }
    for (unsigned src_cp_idx = 0u; src_cp_idx < CPset.size(); ++src_cp_idx) {
        for (unsigned dst_cp_idx = 0u; dst_cp_idx < CPset.size(); ++dst_cp_idx) {
            auto src_idx = CPset[src_cp_idx];
            auto dst_idx = CPset[dst_cp_idx];
            if (src_idx != dst_idx && tg_.comm_size[src_idx][dst_idx] > 0) {
                OperationType src_opt = tg_.tasks[src_idx].op.type;
                OperationType dst_opt = tg_.tasks[dst_idx].op.type;
                if (src_opt != dst_opt) {
                    auto idx = std::make_pair(src_opt, dst_opt);
                    if (op2op_comm_size.count(idx) == 0) {
                        op2op_comm_size[idx] = tg_.comm_size[src_idx][dst_idx];
                    } else {
                        op2op_comm_size[idx] += tg_.comm_size[src_idx][dst_idx];
                    }
                }
            }
        }
    }

    std::unordered_map<OperationType, unsigned> opt_base;
    std::unordered_map<OperationType, unsigned> opt_idx;
    std::unordered_map<OperationType, unsigned> opt_size;
    unsigned size = 1;
    for (auto i : p_.opt_fu_idx) {
        if (op_comp_size.count(i.first) != 0) {
            opt_base[i.first] = size;
            opt_idx[i.first] = 0;
            opt_size[i.first] = i.second.size();
            size *= opt_size[i.first];
        }
    }

    float min_time = FLT_MAX;
    std::unordered_map<OperationType, unsigned> min_opt;
    for (unsigned case_idx = 0; case_idx < size; ++case_idx) {
        for (auto i : opt_idx) {
            opt_idx[i.first] = (case_idx / opt_base[i.first]) % opt_size[i.first];
        }
        float tmp_time = 0;
        for (auto item : op_comp_size) {
            unsigned fu_idx = p_.opt_fu_idx[item.first][opt_idx[item.first]];
            tmp_time += item.second / p_.fu_info[fu_idx].speed;
        }
        for (auto item : op2op_comm_size) {
            OperationType src_type = item.first.first;
            OperationType dst_type = item.first.second;
            unsigned src_fu_idx = p_.opt_fu_idx[src_type][opt_idx[src_type]];
            unsigned dst_fu_idx = p_.opt_fu_idx[dst_type][opt_idx[dst_type]];
            auto idx = std::make_pair(src_type, dst_type);
            tmp_time += op2op_comm_size[idx] / p_.bandwidth[src_fu_idx][dst_fu_idx];
        }
        if (tmp_time < min_time) {
            min_time = tmp_time;
            min_opt.clear();
            for (auto i : opt_idx) {
                min_opt[i.first] = p_.opt_fu_idx[i.first][opt_idx[i.first]];
            }
        }
    }
    for (auto i : min_opt) {
        for (auto cp_idx = 0; cp_idx < CPset.size(); cp_idx++) {
            auto &task = tg_.tasks[CPset[cp_idx]];
            if (task.op.type == i.first)
                task.fu_idx = i.second;
        }
    }

}
void Scheduler::scheduleCPOP() {
    std::vector<unsigned> CPSet = {0};
    unsigned cur_idx = 0;
    auto back_task = tg_.tasks[CPSet.back()];
    while (back_task.op.type != OUTPUT) {
        unsigned next_idx = -1;
        auto cur_sucs = tg_.successor[cur_idx];
        for (auto task_idx : CPOP_sorted_task_idx_) {
            auto next_iter = find(cur_sucs.begin(), cur_sucs.end(), task_idx);
            if (next_iter != cur_sucs.end()) {
                next_idx = *next_iter;
                break;
            }
        }
        CPSet.push_back(next_idx);
        cur_idx = next_idx;
        back_task = tg_.tasks[next_idx];
    }
    setCPsetMinFu(CPSet);

    // processor selection
    std::vector<unsigned> unschedule_tasks = {0};
    std::vector<bool> scheduled(tg_.tasks.size(), false);

    while (!unschedule_tasks.empty()) {
        float max_prio = FLT_MIN;
        unsigned max_prio_task_idx = UINT_MAX;
        for (auto candi_task_idx : unschedule_tasks) {
            auto &candi_task = tg_.tasks[candi_task_idx];
            bool candi_ready = true;
            for (auto pre_task_idx : tg_.precedence[candi_task_idx]) {
                candi_ready = candi_ready && scheduled[pre_task_idx];
            }
            if (candi_ready) {
                if (upward_rank_[candi_task_idx] + downward_rank_[candi_task_idx] > max_prio) {
                    max_prio = upward_rank_[candi_task_idx] + downward_rank_[candi_task_idx];
                    max_prio_task_idx = candi_task_idx;
                }
            } else {
                continue;
            }
        }
        assert(max_prio_task_idx < UINT_MAX);

        auto cur_schedule_idx = max_prio_task_idx;
        auto &cur_schedule_task = tg_.tasks[max_prio_task_idx];

        float min_eft = FLT_MAX;
        unsigned min_fu_idx = UINT_MAX;
        std::vector<unsigned> candi_fus;
        auto find_iter = find(CPSet.begin(), CPSet.end(), cur_schedule_idx);
        if (find_iter != CPSet.end())
            candi_fus.push_back(cur_schedule_task.fu_idx);
        else
            candi_fus = p_.opt_fu_idx[cur_schedule_task.op.type];
        for (auto fu_idx : candi_fus) {
            // given fu idx, compute est with pres tasks
            float min_est = FLT_MAX;
            for (auto pre_task_idx : tg_.precedence[cur_schedule_idx]) {
                auto pre_task = tg_.tasks[pre_task_idx];
                auto speed = p_.bandwidth[pre_task.fu_idx][fu_idx];
                min_est = std::min(min_est, pre_task.finish_time + pre_task.out_size / speed);
            }
            if (tg_.precedence[cur_schedule_idx].empty())
                min_est = 0;
            // update avail_j with min_est
            auto &fu_j = p_.fu_info[fu_idx];
            float min_avail = FLT_MAX;
            for (int insert_pos = fu_j.task_items.size(); insert_pos >= 0; insert_pos--) {
                // check independence
                if (insert_pos < fu_j.task_items.size()) {
                    auto pres = tg_.precedence[cur_schedule_idx];
                    auto post_idx = fu_j.task_items[insert_pos].task_idx;
                    if (std::find(pres.begin(), pres.end(), post_idx) != pres.end())
                        break;
                }
                float avail;
                float length;
                if (insert_pos > 0)
                    avail = fu_j.task_items[insert_pos - 1].finish_time;
                else
                    avail = 0;
                if (insert_pos == fu_j.task_items.size())
                    length = FLT_MAX;
                else
                    length = fu_j.task_items[insert_pos].start_time - avail;
                // assert(avail <= min_avail);
                float dura_length = cur_schedule_task.comp_size / fu_j.speed;
                if (avail >= min_est && length >= dura_length) {
                    min_avail = avail;
                } else if (avail <= min_eft && (avail + length >= min_eft + dura_length)) {
                    min_avail = avail;
                }
            }
            // update actual est
            min_est = std::max(min_avail, min_est);
            auto temp_eft = min_est + cur_schedule_task.comp_size / fu_j.speed;
            if (temp_eft < min_eft) {
                min_eft = temp_eft;
                min_fu_idx = fu_idx;
            }
        }
        assert(min_fu_idx < UINT_MAX);
        tg_.tasks[cur_schedule_idx].fu_idx = min_fu_idx;
        tg_.tasks[cur_schedule_idx].finish_time = min_eft;
        tg_.tasks[cur_schedule_idx].start_time = min_eft - (cur_schedule_task.comp_size / p_.fu_info[min_fu_idx].speed);
        TaskItem cur;
        cur.task_idx = cur_schedule_idx;
        cur.start_time = tg_.tasks[cur_schedule_idx].start_time;
        cur.finish_time = tg_.tasks[cur_schedule_idx].finish_time;
        p_.fu_info[min_fu_idx].insertTaskItem(cur);

        // delete cur_task from unscheduled and set scheduled
        auto cur_iter = find(unschedule_tasks.begin(), unschedule_tasks.end(), cur_schedule_idx);
        assert(cur_iter != unschedule_tasks.end());
        unschedule_tasks.erase(cur_iter);
        scheduled[cur_schedule_idx] = true;
        // insert cur_task's pres to unscheduled tasks
        for (auto pre_idx : tg_.successor[cur_schedule_idx]) {
            if (scheduled[pre_idx]) {
                continue;
            } else if (find(unschedule_tasks.begin(), unschedule_tasks.end(), pre_idx) == unschedule_tasks.end()) {
                unschedule_tasks.push_back(pre_idx);
            }
        }
    }
}

TaskGraph Scheduler::splitTaskByHardwareNum(TaskGraph tg) {
    TaskGraph new_tg;
    std::vector<TaskNode> new_tasks;
    std::vector<TaskNode> &old_tasks = tg.tasks;
    std::vector<int> split_nr;
    std::vector<int> split_idx;

    // construct new tasks
    for (auto ori_idx = 0; ori_idx < old_tasks.size(); ori_idx++) {
        int max_nr = (int )(old_tasks[ori_idx].in_size / old_tasks[ori_idx].op.min_size);
        int fu_nr = p_.opt_fu_idx[old_tasks[ori_idx].op.type].size();
        int sp_nr = std::min(max_nr, fu_nr);
        split_nr.push_back(std::min(max_nr, fu_nr));
        float in_size = old_tasks[ori_idx].in_size / sp_nr;
        float comp_size = old_tasks[ori_idx].comp_size / sp_nr;
        float out_size = old_tasks[ori_idx].out_size / sp_nr;
        for (auto sp_idx = 0; sp_idx < sp_nr; sp_idx++) {
            auto new_task = old_tasks[ori_idx];
            new_task.in_size = in_size;
            new_task.comp_size = comp_size;
            new_task.out_size = out_size;
            new_task.ori_idx = ori_idx;
            new_tasks.push_back(new_task);
            split_idx.push_back(sp_idx);
        }
    }

    // link new tasks
    int new_task_nr = new_tasks.size();
    std::vector<std::vector<float>>new_comm_size(new_task_nr, std::vector<float>(new_task_nr, -1));
    std::vector<std::vector<float>> &old_comm_size = tg.comm_size;
    for (auto new_idx = 0; new_idx < new_task_nr; new_idx++) {
        for (auto suc_idx = 0; suc_idx < new_task_nr; suc_idx++) {
            if (old_comm_size[new_tasks[new_idx].ori_idx][new_tasks[suc_idx].ori_idx] >= 0) {
                if(old_tasks[new_tasks[new_idx].ori_idx].out_size != old_tasks[new_tasks[suc_idx].ori_idx].in_size &&
                   old_tasks[new_tasks[suc_idx].ori_idx].op.type != BINARY &&
                   old_tasks[new_tasks[suc_idx].ori_idx].op.type != CONCAT) {
                    int new_ori_idx = new_tasks[new_idx].ori_idx;
                    int suc_ori_idx = new_tasks[suc_idx].ori_idx;
                    assert(0);
                }
                float new_start_offset, new_end_offset;
                new_start_offset = split_idx[new_idx] * new_tasks[new_idx].out_size;
                new_end_offset = new_start_offset + new_tasks[new_idx].out_size;
                if (new_tasks[suc_idx].op.type == CONCAT ||
                new_tasks[suc_idx].op.type == BINARY) {
                    for (auto ori_idx = 0; ori_idx < old_tasks.size(); ori_idx++) {
                       if (old_comm_size[ori_idx][new_tasks[suc_idx].ori_idx] >= 0 &&
                       ori_idx < new_tasks[new_idx].ori_idx) {
                          new_start_offset += old_tasks[ori_idx].out_size;
                          new_end_offset += old_tasks[ori_idx].out_size;
                       }
                    }
                }
                float suc_start_offset, suc_end_offset;
                suc_start_offset = split_idx[suc_idx] * new_tasks[suc_idx].in_size;
                suc_end_offset = suc_start_offset + new_tasks[suc_idx].in_size;
                float start = std::max(new_start_offset, suc_start_offset);
                float end = std::min(new_end_offset, suc_end_offset);
                // TODO(pengshaohui): deal zero comms_ize
                if (end > start) {
                    new_comm_size[new_idx][suc_idx] = end - start;
                }
            }
        }
    }
    new_tg.comm_size = new_comm_size;
    new_tg.tasks = new_tasks;
    return new_tg;
}
void Scheduler::concatTaskOnSameFu() {
    for (auto fu_idx = 0; fu_idx < p_.fu_info.size(); fu_idx++) {
        auto &cur_fu = p_.fu_info[fu_idx];
        for (auto item_iter = cur_fu.task_items.begin() + 1;
        item_iter != cur_fu.task_items.end();
        item_iter++) {
            if ((*item_iter).start_time == (*(item_iter - 1)).finish_time) {
                unsigned last_idx = (*(item_iter - 1)).task_idx;
                unsigned cur_idx = (*item_iter).task_idx;
                (*(item_iter - 1)).finish_time = (*item_iter).finish_time;
                item_iter = cur_fu.task_items.erase(item_iter);
            }
        }
    }
}
