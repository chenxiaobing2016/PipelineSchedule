#ifndef INCLUDE_LAYERS_LAYERPARAM_H_
#define INCLUDE_LAYERS_LAYERPARAM_H_
struct LayerParam {
 int n, c, h, w;
};

struct ConvParam : public LayerParam {
 int co, kh, hw, sh, sw;
};

struct PoolParam : public LayerParam {
 int kh, hw, sh, sw;
};

struct ReluParam : public LayerParam {};

struct SoftmaxParam : public LayerParam {};

using LayerParamSptr = std::shared_ptr<LayerParam>;

#endif  // INCLUDE_LAYERS_LAYERPARAM_H_
