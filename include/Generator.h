#include <vector>

#include "Processor.h"

class Generator {
public:
  Generator(int h, int w, int o_d, int o_r) : height(h), width(w), out_degree(o_d), out_rate(o_r) {}

  std::vector<Task> genTaskDAG();

  // void setHeight(int h);
  // int getHeight();
  // void setWidth(int w);
  // int getWidth();
  // void setOutDegree(int o_d);
  // int getOutDegree();
  // void setOutRate(int o_r);
  // float getOutRate();

private:
  struct OpGrid{
    OperationType ot;
    std::vector<int> pres;
    std::vector<int> sucs;
  };
  float input_size;
  int height;
  int width;
  int out_degree;
  float out_rate;
};
