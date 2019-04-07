#ifndef PIPELINESCHEDULE_FUNCTIONUNIT_H_
#define PIPELINESCHEDULE_FUNCTIONUNIT_H_

#include <vector>

#include "Util.h"

enum OperationType{
  INPUT = 0,
  OUTPUT = 1,
  // EMPTY = 2,
  CONV = 2,
  POOL = 3,
  FC = 4,
  ACTIVE = 5,
  BINARY = 6,
  CONCAT = 7,
  SLICE = 8
};
struct Operation {
  OperationType type;
  float min_size;
};

struct FU {
  float speed;
  Operation op;
};

struct Processor {
  std::vector<std::pair<FU, int>> fu_info;
  std::vector<std::vector<float>> bandwith;
};

struct Task {
  float in_size;
  float out_size;
  // std::function<float(float)> in_out;
  std::vector<Task*> precursors;
  std::vector<Task*> successors;
  Operation op;
};

#endif  // PIPELINESCHEDULE_FUNCTIONUNIT_H_
