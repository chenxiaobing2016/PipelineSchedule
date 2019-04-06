#ifndef PIPELINESCHEDULE_FUNCTIONUNIT_H_
#define PIPELINESCHEDULE_FUNCTIONUNIT_H_

#include <vector>

#include "Util.h"

enum OperationType{
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
  int id;
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
  std::function<float(float)> in_out;
  std::vector<Task*> precursors;
  std::vector<Task*> successors;
  Operation op;
};

#endif  // PIPELINESCHEDULE_FUNCTIONUNIT_H_
