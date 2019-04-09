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

template<>
struct std::hash<OperationType> {
  size_t operator()(const OperationType& opt) const {
    return (size_t)opt;
  }
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
  std::unordered_map<OperationType, std::vector<unsigned>> opt_fu_idx;

  void setOperatorType2FuIdx();
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
  bool existDependence(unsigned a, unsigned b);

  bool isAncestor(unsigned a, unsigned b);
};


#endif  // PIPELINESCHEDULE_FUNCTIONUNIT_H_
