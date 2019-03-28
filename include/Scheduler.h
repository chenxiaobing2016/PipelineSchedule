#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <array>
#include <vector>

#include "FunctionUnit.h"
#include "WorkLoad.h"

class Scheduler {
 public:
  schedule() { NOT_IMPLEMENTED }
 private:
  std::vector<std::Array<StageWorkLoadSptr, 3>> wl_;
  Processor proc_;
};

#endif  // INCLUDE_SCHEDULER_H_
