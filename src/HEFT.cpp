#include <unordered_map>
#include <unordered_set>

#include "HEFT.h"

/*
HEFT::HEFT(std::vector<Task> tasks, Processor p)
        : tasks_(tasks), p_(p), l_(0)
{}

void HEFT::run() {
  reckonAvgCompCost();
  reckonAvgCommCost();
  reckonUpwardRank();
}

void HEFT::reckonUpwardRank() {
  int task_nr = tasks_.size();
  upward_rank_.resize(task_nr, 0);
  std::unordered_set<Task> task_set;

  int set_cnt = 0;
  while (set_cnt < task_nr) {
    for (unsigned idx = 0u; idx < task_nr; ++idx) {
      if (task_set.find(tasks_[idx]) != task_set.end()) { continue; }
      bool no_successor = true;
      for (Task* task_ptr : tasks_[idx].successors) {
        if (task_set.find(*task_ptr) == task_set.end()) {
          no_successor = false;
          break;
        }
      }
      if (no_successor) {
        for ()
        upward_rank_[idx] =
        ++set_cnt;
      }
    }
  }
}

void HEFT::reckonAvgCompCost() {
  avg_comp_cost_.resize(tasks_.size());
  std::unordered_map<Operation, std::vector<float>> fu_speed;
  for (auto tmp : p_.fu_info) {
    Operation op = tmp.first.op;
    float speed = tmp.first.speed;
    fu_speed[op].push_back(speed);
  }
  for (unsigned i = 0u; i < avg_comp_cost_.size(); ++i) {
    std::vector<float> speeds = fu_speed[tasks_[i].op];
    float avg_speed = std::accumulate(speeds.begin(), speeds.end(), 0);
    avg_speed /= speeds.size();
    avg_comp_cost_[i] = tasks_[i].in_size / avg_speed;
  }
}

void HEFT::reckonAvgCommCost() {
  int task_nr = tasks_.size();
  std::vector<std::vector<float>> comm_size;
  comm_size.resize(task_nr, std::vector<float>(task_nr, 0));
  avg_comm_cost_.resize(task_nr, std::vector<float>(task_nr, 0));

  setCommSize(comm_size);

  for (unsigned i = 0u; i < task_nr; ++i) {
    std::vector<unsigned> src_fu_idx = getFuIdxByOperation(tasks_[i].op);
    for (unsigned j = 0u; j < task_nr; ++j) {
      std::vector<unsigned> dst_fu_idx = getFuIdxByOperation(tasks_[j].op);
      comm_size[i][j] = 0;
      if (comm_size[i][j] != 0) {
        float avg_bd = 0;
        for (auto src_idx = 0; src_idx < src_fu_idx.size(); ++src_idx) {
          for (auto dst_idx = 0; dst_idx < dst_fu_idx.size(); ++dst_idx) {
            avg_bd += p_.bandwith[src_fu_idx[src_idx]][dst_fu_idx[dst_idx]];
          }
        }
        avg_bd /= (src_fu_idx.size() * dst_fu_idx.size());
        avg_comm_cost_[i][j] = l_ + comm_size[i][j] / avg_bd;
      }
    }
  }
}

void HEFT::setCommSize(std::vector<std::vector<float>>& commSize) {
  unsigned task_nr = tasks_.size();
  for (unsigned i = 0; i < task_nr; ++i) {
    for (unsigned j = 0; j < task_nr; ++j) {
      Task src_task = tasks_[i];
      Task dst_task = tasks_[j];
      auto src_successors = src_task.successors;
      auto iter = std::find(src_successors.begin(), src_successors.end(), &dst_task);
      if (iter == src_successors.end()) {
        commSize[i][j] = 0;
        continue;
      }
      switch(dst_task.op) {
        case CONCAT: commSize[i][j] = dst_task.in_size / dst_task.precursors.size(); break;
        default: commSize[i][j] = dst_task.in_size;
      }
    }
  }
}

std::vector<unsigned> HEFT::getFuIdxByOperation(Operation op) {
  std::vector<unsigned> ret_val;
  auto fu_info = p_.fu_info;
  for (unsigned i = 0; i < fu_info.size(); ++i) {
    if (fu_info.first.op == op) {
      ret_val.push_back(i);
    }
  }
  return ret_val;
}
 */
