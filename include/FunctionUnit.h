#ifndef INCLUDE_FUNCTIONUNIT_H_
#define INCLUDE_FUNCTIONUNIT_H_

#include <vector>

#include "Type.h"
#include "Util.h"

class FunctionUnit {
  FUType type_;
  float speed_;
 public:
  FunctionUnit(FUType type, float speed)
      : type_(type), speed_(speed) {}
  GET_SET_ACCESS(FUType, Type, type_)
  GET_SET_ACCESS(float, Speed, speed_)
  float getTime(float size) { return size / speed_; }
};

class Processor {
 public:
  void config() {
    IOFUs_.push_back(FunctionUnit(FUType::kNeuron,  10));
    IOFUs_.push_back(FunctionUnit(FUType::kSynapse, 20));
    IOFUs_.push_back(FunctionUnit(FUType::kConst,   30));

    CPFUs_.push_back(FunctionUnit(FUType::kScalar, 1000));
    CPFUs_.push_back(FunctionUnit(FUType::kStream, 32));
    CPFUs_.push_back(FunctionUnit(FUType::kMatrix, 16 * 32));
  }
 private:
  std::vector<FunctionUnit> IOFUs_;
  std::vector<FunctionUnit> CPFUs_;
};

#endif  // INCLUDE_FUNCTIONUNIT_H_
