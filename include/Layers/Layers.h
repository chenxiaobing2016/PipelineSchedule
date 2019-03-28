#ifndef INCLUDE_LAYERS_LAYERS_H_
#define INCLUDE_LAYERS_LAYERS_H_

#include <vector>

#include "Layers/LayerParam.h"
#include "WorkLoad.h"

#define ADD_LAYER(LayerName)                                        \
  class LayerName##Layer : public BaseLayer {                       \
   public:                                                          \
    std::vector<OpWorkLoadSptr> genWorkLoad(LayerParamSptr) override;\
  };

class BaseLayer {
 public:
  virtual std::vector<OpWorkLoadSptr> genWorkLoad(LayerParamSptr) = 0;
};

ADD_LAYER(Conv)
ADD_LAYER(Pool)
ADD_LAYER(Relu)
ADD_LAYER(Softmax)

std::vector<OpWorkLoadSptr> ConvLayer::genWorkLoad(LayerParamSptr) {
  NOT_IMPLEMENTED
}

std::vector<OpWorkLoadSptr> PoolLayer::genWorkLoad(LayerParamSptr) {
  NOT_IMPLEMENTED
}

std::vector<OpWorkLoadSptr> ReluLayer::genWorkLoad(LayerParamSptr) {
  NOT_IMPLEMENTED
}

std::vector<OpWorkLoadSptr> SotmaxLayer::genWorkLoad(LayerParamSptr) {
  NOT_IMPLEMENTED
}

#endif  // INCLUDE_LAYERS_LAYERS_H_
