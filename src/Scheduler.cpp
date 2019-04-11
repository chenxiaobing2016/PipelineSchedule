#include <algorithm>
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

void Scheduler::reckonAvgCompCost() {
  auto tasks = tg_.tasks;
  unsigned task_nr = tasks.size();

  // reckon average computational cost for each task.
  avg_comp_cost_.resize(task_nr, 0);
  for (unsigned task_idx = 0u; task_idx < task_nr; ++task_idx) {
    auto task = tasks[task_idx];
    OperationType opt = task.op.type;
    avg_comp_cost_[task_idx] = tasks[task_idx].in_size / p_.op2avg_comp_speed[opt];
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
            if (reckoned_tasks.find(idx) == reckoned_tasks.end()) {
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
                        + tasks[prec_idx].in_size / p_.op2avg_comp_speed[tasks[prec_idx].op.type]
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
      if (reckoned_tasks.find(idx) == reckoned_tasks.end()) {
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
      for (auto succ_idx : tg_.successor[idx]) {
        float comm_speed = p_.op2op_avg_comm_speed[std::make_pair(tasks[idx].op.type,
                                                   tasks[succ_idx].op.type)];
        float tmp = tg_.comm_size[idx][succ_idx] / comm_speed + upward_rank_[succ_idx];
        upward_rank_[idx] = std::max(upward_rank_[idx], tmp);
      }
      upward_rank_[idx] += tasks[idx].in_size / p_.op2avg_comp_speed[tasks[idx].op.type];
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
  auto tasks = tg_.tasks;
  unsigned task_nr = tasks.size();

  std::vector<std::vector<unsigned>> fu2task_idx(p_.fu_info.size(), std::vector<unsigned>());
  for (unsigned task_idx = 0; task_idx < task_nr; ++task_idx) {
    auto task = tasks[HEFT_sorted_task_idx_[task_idx]];
    auto opt = task.op.type;
    std::vector<unsigned> fu_condidates = p_.opt_fu_idx[opt];

    unsigned fu_id = -1;
    float start_time, finish_time;

    for (auto fu_idx : fu_condidates) {
      // reckon avail_time
      float avail_time = 0;
      float cpt_time = task.in_size / p_.fu_info[fu_idx].speed;
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
          if (i == 0 && task_items[0].start_time >= cpt_time) {
            avail_time = 0;
            break;
          } else if (task_items[i].start_time - task_items[i - 1].finish_time >= cpt_time) {
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
