#ifndef PIPELINESCHEDULE_FUNCTIONUNIT_H_
#define PIPELINESCHEDULE_FUNCTIONUNIT_H_

#include <queue>
#include <unordered_map>
#include <vector>

#include "Util.h"
enum NetType {
    LENET,
    ALEXNET,
    VGG,
    GOOGLENET,
    RESNET
};

enum OperationType {
  INPUT = 0,
  OUTPUT = 1,
  EMPTY = 2,
  CONV = 3,
  POOL = 4,
  FC = 5,
  ACTIVE = 6,
  BINARY = 7,
  CONCAT = 8,
  SLICE = 9
};
struct Operation {
  OperationType type;
  float min_size;
  bool operator==(Operation& op) {
    return (this->type == op.type)
           && (this->min_size == op.min_size);
  }
};

struct FU {
  float speed;
  Operation op;
};

struct Processor {
  std::vector<FU> fu_info;
  std::vector<std::vector<float>> bandwith;

  // assisted data structure.
  std::unordered_map<OperationType, std::vector<unsigned>> opt_fu_idx;
  void setOperatorType2FuIdx() {
    static bool has_set = false;
    if (!has_set) {
      has_set = true;
      for (auto idx = 0u; idx < fu_info.size(); ++idx) {
        OperationType opt = fu_info[idx].op.type;
        opt_fu_idx[opt] = idx;
      }
    }
  }
};

struct TaskNode {
  float in_size;
  float out_size;
  Operation op;
};

struct TaskGraph {
  std::vector<TaskNode> tasks;
  std::vector<std::vector<float>> comm_size;


  // assisted data structure.
  unsigned fu_idx;
  unsigned start_time, end_time;

  bool existDependence(unsigned a, unsigned b) {
    return isAncestor(a, b) || isAncestor(b, a);
  }

  bool isAncestor(unsigned a, unsigned b) {
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
};


#endif  // PIPELINESCHEDULE_FUNCTIONUNIT_H_
