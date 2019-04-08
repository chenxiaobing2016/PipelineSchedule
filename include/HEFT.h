#ifndef PIPELINESCHEDULE_HEFT_H
#define PIPELINESCHEDULE_HEFT_H

#include <vector>

#include "Processor.h"

class HEFT {
public:
  HEFT(TaskGraph tg, Processor p) : tg_(tg), p_(p), l_(0) {}

  void run() {
    reckonAvgCompCost();
    reckonAvgCommCost();
    reckonUpwardRank();
    sortByUpwardRank();
  }

private:
  void reckonAvgCompCost() {
    auto tasks = tg_.tasks;
    unsigned task_nr = tasks.size();
    avg_comp_cost_.resize(task_nr, 0);
    std::unordered_map<OperationType, float> op2avg_compt_speed;
    for (auto i : p_.opt_fu_idx) {
      float speed = 0;
      for (auto idx : i.second) {
        speed += p_.fu_info[idx].speed;
      }
      op2avg_compt_speed[i.first] = speed / i.second.size();
    }

    for (unsigned task_idx = 0u; task_idx < task_nr; ++task_idx) {
      auto task = tasks[task_idx];
      OperationType opt = task.op.type;
      avg_comp_cost_[task_idx] = tasks[task_idx].in_size / op2avg_compt_speed[opt];
    }
  }

  void reckonAvgCommCost() {
    auto tasks = tg_.tasks;
    unsigned task_nr = tasks.size();
    avg_comm_cost_.resize(task_nr, std::vector<float>(task_nr, 0));

    for (unsigned src_idx = 0; src_idx < task_nr; ++src_idx) {
      for (unsigned dst_idx = 0; dst_idx < task_nr; ++dst_idx) {
        if (src_idx != dst_idx && tg_.comm_size[src_idx][dst_idx] != -1) {
          auto src_opt = tasks[src_idx].op.type;
          auto dst_opt = tasks[dst_idx].op.type;
          auto src_fus_idx = p_.opt_fu_idx[src_opt];
          auto dst_fus_idx = p_.opt_fu_idx[dst_opt];
          float avg_bd = 0;
          for (auto src_fu_idx : src_fus_idx) {
            for (auto dst_fu_idx : dst_fus_idx) {
              avg_bd += p_.bandwith[src_fu_idx][dst_fu_idx];
            }
          }
          avg_bd /= (src_fus_idx.size() * dst_fus_idx.size());
          avg_comm_cost_[src_idx][dst_idx] = l_ + tg_.comm_size[src_idx][dst_idx] /avg_bd;
        }
      }
    }
  }

  void reckonUpwardRank() {
    auto tasks = tg_.tasks;
    auto task_nr = tasks.size();

    upward_rank_.resize(task_nr, 0);
    int cnt = 0;
    std::vector<bool> flag(task_nr, true);
    while (cnt < tasks.size()) {
      bool valid = true;
      for (unsigned idx = 0; idx < task_nr; ++idx) {
        if (!flag[idx]) { continue; }
        for (unsigned succ_idx = 0; succ_idx < task_nr; ++succ_idx) {
          if (tg_.comm_size[idx][succ_idx] != -1 && flag[idx]) {
            valid = false;
            break;
          }
        }
        if (valid) {
          ++cnt;
          flag[idx] = false;
          upward_rank_[idx] += avg_comp_cost_[idx];
          float tmp = 0;
          for (unsigned succ_idx = 0; succ_idx < task_nr; ++succ_idx) {
            if (tg_.comm_size[idx][succ_idx] != -1) {
              tmp = std::max(tmp, avg_comm_cost_[idx][succ_idx] + upward_rank_[succ_idx]);
            }
          }
          upward_rank_[idx] += tmp;
        }
      }
    }
  }

  void sortByUpwardRank() {
    auto tasks = tg_.tasks;
    unsigned task_nr = tasks.size();
    sorted_task_idx_.resize(task_nr, 0);
    for (auto i = 0; i < task_nr; ++i) {
      sorted_task_idx_[i] = i;
    }
    auto ur = upward_rank_;
    auto cmp = [ur](unsigned a, unsigned b) {
      return ur[a] >= ur[b];
    };
    std::sort(sorted_task_idx_.begin(), sorted_task_idx_.end(), cmp);
  }

  void schedule() {
    auto tasks = tg_.tasks;
    unsigned task_nr = tasks.size();
    std::vector<std::vector<unsigned>> fu2task_idx(p_.fu_info.size(), std::vector<unsigned>());
    for (unsigned task_idx = 0; task_idx < task_nr; ++task_idx) {
      auto opt = tasks[task_idx].op.type;
      std::vector<unsigned> fu_condidates = p_.opt_fu_idx[opt];
      for (auto idx : fu_condidates) {
      }  // for idx
    }
  }

  std::vector<std::vector<float>> avg_comm_cost_;
  std::vector<float> avg_comp_cost_;
  std::vector<float> upward_rank_;
  std::vector<unsigned> sorted_task_idx_;



  TaskGraph tg_;
  Processor p_;
  // average communication startup time
  float l_;
};

#endif //PIPELINESCHEDULE_HEFT_H
