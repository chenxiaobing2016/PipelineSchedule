#ifndef INCLUDE_TYPE_H_
#define INCLUDE_TYPE_H_

enum class FUType {
  kNeuron,
  kSynapse,
  kConst,
  kScalar,
  kStream,
  kMatrix,
};

enum class Stage {
  kLoad,
  kCompute,
  kStore,
};

#endif  // INCLUDE_TYPE_H_
