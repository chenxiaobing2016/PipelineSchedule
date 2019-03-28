#ifndef INCLUDE_WORKLOAD_H_
#define INCLUDE_WORKLOAD_H_

#include <vector>

#include "Type.h"
#include "Util.h"

ENABLE_SHARED_CLASS(OpWorkLoad) {
 public:
  OpWorkLoadSptr getSptr() { return std::shared_from_this(); }
 private:
  float size_;
  FUType type_;
  std::vector<OpWorkLoadWptr> deps_;
};

ENABLE_SHARED_CLASS(StageWorkLoad) {
 public:
  StageWorkLoadSptr getSptr() { return std::shared_from_this(); }
 private:
  Stage stage_;
  std::vector<OpWorkLoadSptr> ops_;
};

#endif  // INCLUDE_WORKLOAD_H_
