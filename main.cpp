#include <iostream>
#include "include/Generator.h"
using namespace std;
int main() {
    float input_size = 1000.0;
    int v = 1000;
    float alpha = 0.5;  // (0, 1]
    int task_graph_nr = 3;
    int out_degree = 8;
    float out_rate = 0.8;  // (0, 1]
    float speed_rate = 0.5;  // (0, 1];
    float ccr = 0.5;  // (0, 1]
    auto test_generator = Generator(input_size,
                                    v,
                                    alpha,
                                    task_graph_nr,
                                    out_degree,
                                    out_rate,
                                    speed_rate,
                                    ccr);
    test_generator.genRandomTaskDAGs();
    // auto task_graphs = test_generator.getTaskGraphs();

    // cout << "correctness: " << test_generator.checkCycleAndConnect(task_graphs[2]) << endl;
    // test_generator.printTaskDAGs();


    // fu_info in processor need set
    // op in FU need set
    // example:
    Processor test_processor;
    // Q1. different type of op need different number and speed rate hardware;
    // A1. may solved by set different func: pre's out_size -> in_size
    // Q2. some type of op should shared same hardware, like SLICE and CONCAT
    // A2. ?
    int input_nr = 1;
    int output_nr = 1;
    int conv_nr = 2;
    int fc_nr = 2;
    int pool_nr = 2;
    int active_nr = 2;
    int binary_nr = 2;
    int concat_nr = 3;
    int slice_nr = 2;
    vector<int> fu_nr = {input_nr, output_nr, conv_nr, pool_nr, fc_nr, active_nr, binary_nr, concat_nr, slice_nr};
    for (int op_idx = 0; op_idx < 9; op_idx++) {
        for (auto fu_idx = 0; fu_idx < fu_nr[op_idx]; fu_idx++) {
            FU fu;
            fu.op.type = (OperationType)op_idx;
            test_processor.fu_info.push_back(fu);
        }
    }
    test_generator.genSpeedTable(test_processor);
    test_generator.printSpeedTable(test_processor);
    return 0;
}
