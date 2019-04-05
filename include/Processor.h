#ifndef PIPELINESCHEDULE_FUNCTIONUNIT_H_
#define PIPELINESCHEDULE_FUNCTIONUNIT_H_

#include <vector>

#include "Util.h"

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
