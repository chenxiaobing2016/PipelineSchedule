#include "Processor.h"

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

bool TaskGraph::existDependence(unsigned a, unsigned b) {
  return isAncestor(a, b) || isAncestor(b, a);
}

bool TaskGraph::isAncestor(unsigned a, unsigned b) {
  std::queue<unsigned> qu;
  qu.push(a);
  while (!qu.empty()) {
    for (auto i = 0; i < tasks.size(); ++i) {
      if (comm_size[qu.front()][i] != -1) {
        if (i == b) {
          return true;
        }
        qu.push(i);
        qu.pop();
      }
    }
  }
  return false;
}
