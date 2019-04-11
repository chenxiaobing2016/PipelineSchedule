#ifndef PIPELINESCHEDULE_FUNCTIONUNIT_H_
#define PIPELINESCHEDULE_FUNCTIONUNIT_H_

#include <climits>
#include <queue>
#include <unordered_map>
#include <vector>
#include <cassert>

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

template<>
struct std::hash<std::pair<OperationType, OperationType>> {
  size_t operator()(const std::pair<OperationType, OperationType>& in) const {
    return (size_t) in.first * INT_MAX + in.second;
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

// used to record a task on a fu.
struct TaskItem {
  unsigned task_idx;
  float start_time, finish_time;
};

struct FU {
  float speed;
  Operation op;

  // task_items is sorted by start time.
  std::vector<TaskItem> task_items;
  void insertTaskItem(TaskItem task_item);
};

struct Processor {
  std::vector<FU> fu_info;
  std::vector<std::vector<float>> bandwidth;

  // assisted data structure.
  std::unordered_map<OperationType, std::vector<unsigned>> opt_fu_idx;

  void setOperatorType2FuIdx();

  std::unordered_map<OperationType, float> op2avg_comp_speed;
  // reckon average computational speed for each kind of op.
  void setAvgCompSpeedForOp();

  std::unordered_map<std::pair<OperationType, OperationType>, float> op2op_avg_comm_speed;
  // reckon communicational speed for each two kinds of op.
  void setAvgCommSpeedForOp2Op();
};

struct TaskNode {
  float in_size;
  float out_size;
  Operation op;

  // assisted data structure.
  unsigned fu_idx;
  float start_time, finish_time;
  void setFuInfo(unsigned fi, float st, float ft) {
    fu_idx = fi;
    start_time = st;
    finish_time = ft;
  }
};

struct TaskGraph {
  std::vector<TaskNode> tasks;
  std::vector<std::vector<float>> comm_size;


  // assisted data structure.
  // whether there exists a path between tasks[a] and tasks[b].
  bool existDependence(unsigned a, unsigned b);
  // whether tasks[a] is the ancestor of tasks[b]
  bool isAncestor(unsigned a, unsigned b);

  std::vector<std::vector<unsigned>> precedence;
  std::vector<std::vector<unsigned>> successor;
  unsigned entry, exit;
  // set precedence successor for each task,
  // and set entry/exit point for the graph.
  void setTaskRelation();
};


#endif  // PIPELINESCHEDULE_FUNCTIONUNIT_H_
