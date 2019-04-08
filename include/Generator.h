#include <vector>

#include "Processor.h"

class Generator {
public:
  Generator(float i_s, int h, int w, int o_d, float o_r,
            float s_r = 1, float ccr = 1)
            : input_size(i_s), height(h), width(w), out_degree(o_d), out_rate(o_r),
              speed_rate(ccr), ccr(ccr) {}

  // generate random task dag
  void genRandomTaskDAG();

  // print TaskDAG
  void printTaskDAG();

  // generate specific nn task dag
  void genNNTaskDAG(NetType nn);

  // generate speed table relate to hardwares
  void genSpeedTable();

  // printSpeedTable
  void printSpeedTable();

  // void setHeight(int h);
  // int getHeight();
  // void setWidth(int w);
  // int getWidth();
  // void setOutDegree(int o_d);
  // int getOutDegree();
  // void setOutRate(int o_r);
  // float getOutRate();

private:
    // parameters for tasks
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

    // parameters speed tables
    float speed_rate;  // given operation type: speed_rate = theory fu speed / average task input size
                       // true fu speed = sample from [0.5 * theory, 1.5 * theory]
                       // ps: 1. fu with same op, speed should be equal
                       //     2. for a never appeared layer in given graph, average task input size = input_size
                       //     3. for input and output not be bottleneck, their fu speed is FLT_MAX
    float ccr;  // given src and dst fu, ccr = theory communicate speed / avg(src theory fu speed, dst theory fu speed)
                // true communicate speed = sample from [0.5 * theory, 1.5 * theory]

    std::vector<Task> tasks;  // task linked list
    std::vector<float> fu_speed_table;  // size: #operationtype * 1
    std::vector<std::vector<float>> comm_speed_table; // size: #operationtype * #operationtype
};
