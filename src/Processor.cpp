#include <iostream>
#include <iomanip>
#include "Processor.h"

std::string netTypeToString(NetType nt) {
  switch (nt) {
    case LENET     : return std::string("LENET");
    case ALEXNET   : return std::string("ALEXNET");
    case VGG16     : return std::string("VGG16");
    case GOOGLENET : return std::string("GOOGLENET");
    case RESNET18  : return std::string("RESNET18");
    default        : return std::string("UNKNOWN Layer");
  }
}

std::string toString(OperationType opt) {
  switch (opt) {
    case INPUT : return "INPUT";
    case OUTPUT: return "OUTPUT";
    case CONV  : return "CONV";
    case POOL  : return "POOL";
    case FC    : return "FC";
    case ACTIVE: return "ACTIVE";
    case BINARY: return "BINARY";
    case CONCAT: return "CONCAT";
    case SLICE : return "SLICE";
    case EMPTY : return "EMPTY";
    default    : return "UNSUPPORTED TYPE";
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
  opt_fu_idx.clear();
  for (auto idx = 0u; idx < fu_info.size(); ++idx) {
    OperationType opt = fu_info[idx].op.type;
    opt_fu_idx[opt].push_back(idx);
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
  precedence.clear();
  successor.clear();
  precedence.resize(task_nr, std::vector<unsigned>());
  successor.resize(task_nr, std::vector<unsigned>());
  for (unsigned src = 0u; src < tasks.size(); ++src) {
    for (unsigned dst = 0u; dst < tasks.size(); ++dst) {
      if (src != dst && comm_size[src][dst] > 1e-6) {
        // std::cout << "src: " << src << " dst: " << dst \
                  << " " << comm_size[src][dst] << std::endl;
        successor[src].push_back(dst);
        precedence[dst].push_back(src);
      }
    }
  }

#if 0
  std::cout << "precedence:" << std::endl;
  for (unsigned idx = 0u; idx < tasks.size(); ++idx) {
    std::cout << idx << std::endl;
    for (unsigned i = 0; i < precedence[idx].size(); ++i) {
      std::cout << " " << precedence[idx][i] << " " << idx
                << " comm size " << comm_size[precedence[idx][i]][idx] << std::endl;
    }
  }
  std::cout << "successor:" << std::endl;
  for (unsigned idx = 0u; idx < tasks.size(); ++idx) {
    std::cout << idx << std::endl;
    for (unsigned i = 0; i < successor[idx].size(); ++i) {
      std::cout << " " << idx << " " << successor[idx][i]
                << " comm size " << comm_size[idx][successor[idx][i]] << std::endl;
    }
  }
#endif
  entry = exit = -1;
  unsigned entry_cnt = 0, exit_cnt = 0;
  for (unsigned idx = 0u; idx < tasks.size(); ++idx) {
    if (precedence[idx].size() == 0) {
      ++entry_cnt;
      entry = idx;
      // std::cout << "entry: " << entry_cnt << " " << idx << std::endl;
    }
    if (successor[idx].size() == 0) {
      ++exit_cnt;
      exit = idx;
      // std::cout << "exit: " << exit_cnt << " " << idx << std::endl;
    }
  }
  if (entry_cnt != 1 || exit_cnt != 1) {
    std::cout << "entry_cnt: " << entry_cnt << " exit_cnt: " << exit_cnt << std::endl;
    showCommSize();
  }
  assert(entry_cnt == 1 && exit_cnt == 1);
}

void TaskGraph::showCommSize() {
  for (unsigned i = 0; i < comm_size.size(); ++i) {
    for (unsigned j = 0; j < comm_size.size(); ++j) {
      std::cout << std::setw(5) << comm_size[i][j] << " ";
    }
    if (successor[i].size() > 0) {
      std::cout << std::setw(5) << successor[i].size() << " " << successor[i][0] << std::endl;
    } else {
      std::cout << std::setw(5) << successor[i].size() << " " << std::endl;
    }
  }
}
