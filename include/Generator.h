#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <float.h>
#include <math.h>

#include "Processor.h"

class Generator {
public:
  Generator(float i_s_, int v_, float alpha_, int task_graph_nr_, int o_d, float o_r,
            float s_r_ = 1, float ccr_ = 1)
            : input_size(i_s_), v(v_), alpha(alpha_), out_degree(o_d), out_rate(o_r),
              speed_rate(ccr_), ccr(ccr_) {}
  // generate a random task dag
  void genRandomTaskDAG(int height, int width);

  // generate a group of random task dag
  void genRandomTaskDAGs();

  // print TaskDAG
  void printTaskDAGs();

  // generate specific nn task dag
  void genNNTaskDAG(NetType nn);

  // generate speed table relate to hardwares
  void genSpeedTable(Processor &processor);

  // printSpeedTable
  void printSpeedTable(Processor processor);

  // utils
  std::string opTypeToName(OperationType ot);

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
        int id;
        OperationType ot;
        std::vector<int> pres;
        std::vector<int> sucs;
    };
    float input_size;
    int v;
    float alpha;
    int task_graph_nr;
    // int height;
    // int width;
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

    // std::vector<Task> tasks;  // task linked list
    std::vector<TaskGraph> task_graphs;
    // std::vector<float> fu_speed_table;  // size: #operationtype * 1
    // std::vector<std::vector<float>> comm_speed_table; // size: #operationtype * #operationtype
};
