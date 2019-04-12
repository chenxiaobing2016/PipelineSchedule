#include "Processor.h"

std::string toString(OperationType opt) {
  switch (opt) {
    case INPUT : return "INPUT";
    case OUTPUT: return "INPUT";
    case CONV  : return "CONV";
    case POOL  : return "POOL";
    case FC    : return "FC";
    case ACTIVE: return "ACTIVE";
    case BINARY: return "BINARY";
    case CONCAT: return "CONCAT";
    case SLICE : return "SLICE";
    case EMPTY : return "EMPTY";
    default    : return "UNSUPPORT TYPE";
  }
}

void FU::insertTaskItem(TaskItem task_item) {
  int pos = task_items.size() - 1;
  for (; pos >= 0; --pos) {
    if (task_items[pos].start_time < task_item.start_time) {
      break;
    }
  }
  task_items.insert(std::next(task_items.begin(), pos + 1), task_item);
}

void FU::clearTaskItem() {
  task_items.clear();
}

void Processor::setOperatorType2FuIdx() {
  static bool has_set = false;
  if (!has_set) {
    has_set = true;
    for (auto idx = 0u; idx < fu_info.size(); ++idx) {
      OperationType opt = fu_info[idx].op.type;
      opt_fu_idx[opt].push_back(idx);
    }
  }
}

void Processor::setAvgCompSpeedForOp() {
  for (auto i : opt_fu_idx) {
    float speed = 0;
    for (auto idx : i.second) {
      speed += fu_info[idx].speed;
    }
    op2avg_comp_speed[i.first] = speed / i.second.size();
  }
}

void Processor::setAvgCommSpeedForOp2Op() {
  for (auto src : opt_fu_idx) {
    for (auto dst : opt_fu_idx) {
      auto idx = std::make_pair(src.first, dst.first);
      op2op_avg_comm_speed[idx] = 0;
      for (auto src_idx : src.second) {
        for (auto dst_idx : dst.second) {
          op2op_avg_comm_speed[idx] += bandwidth[src_idx][dst_idx];
        }
      }
      op2op_avg_comm_speed[idx] /= (src.second.size() * dst.second.size());
    }
  }
}

bool TaskGraph::existDependence(unsigned a, unsigned b) {
  return isAncestor(a, b) || isAncestor(b, a);
}

bool TaskGraph::isAncestor(unsigned a, unsigned b) {
  std::queue<unsigned> qu;
  qu.push(a);
  while (!qu.empty()) {
    if (qu.front() == b) {
      return true;
    }
    for (auto i : successor[qu.front()]) {
      qu.push(i);
    }
    qu.pop();
  }
  return false;
}

void TaskGraph::setTaskRelation() {
  unsigned task_nr = tasks.size();
  precedence.resize(task_nr, std::vector<unsigned>());
  successor.resize(task_nr, std::vector<unsigned>());
  for (unsigned src = 0u; src < tasks.size(); ++src) {
    for (unsigned dst = 0u; dst < tasks.size(); ++dst) {
      if (src != dst && comm_size[src][dst] != -1) {
        successor[src].push_back(dst);
        precedence[dst].push_back(src);
      }
    }
  }

  entry = exit = -1;
  unsigned entry_cnt = 0, exit_cnt = 0;
  for (unsigned idx = 0u; idx < tasks.size(); ++idx) {
    if (precedence[idx].size() == 0) {
      ++entry_cnt;
      entry = idx;
    }
    if (successor[idx].size() == 0) {
      ++exit_cnt;
      exit = idx;
    }
  }
  assert(entry_cnt == 1 && exit_cnt == 1);
}
