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
  CONV = 2,
  POOL = 3,
  FC = 4,
  ACTIVE = 5,
  BINARY = 6,
  CONCAT = 7,
  SLICE = 8,
  EMPTY = 9
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

  std::vector<unsigned> task_idx;
  std::vector<float> start_time, finish_time;
};

struct Processor {
  std::vector<FU> fu_info;
  std::vector<std::vector<float>> bandwith;

  // assisted data structure.
  // std::unordered_map<OperationType, std::vector<unsigned>> opt_fu_idx;
  // void setOperatorType2FuIdx() {
  //   static bool has_set = false;
  //   if (!has_set) {
  //     has_set = true;
  //     for (auto idx = 0u; idx < fu_info.size(); ++idx) {
  //       OperationType opt = fu_info[idx].op.type;
  //       opt_fu_idx[opt].push_back(idx);
  //     }
  //   }
  // }
};

struct TaskNode {
  float in_size;
  float out_size;
  Operation op;

  // assisted data structure.
  unsigned fu_idx;
  unsigned start_time, finish_time;
};

struct TaskGraph {
  std::vector<TaskNode> tasks;
  std::vector<std::vector<float>> comm_size;


  // assisted data structure.
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
