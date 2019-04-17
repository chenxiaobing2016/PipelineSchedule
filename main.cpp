#include <iostream>
#include <Metrics.h>
#include <Partition.h>
#include "Generator.h"
#include "Scheduler.h"

using namespace std;

void genTaskGraph(std::vector<TaskGraph>& task_graphs, Processor& processor, int nn = -1) {
  float input_size = 1000.0;
  int v = 20;
  float alpha = 0.5;  // (0, 1]
  int task_graph_nr = 3;
  int out_degree = 15;
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
  if (nn == -1) {
    test_generator.genRandomTaskDAGs();
    test_generator.genDAGdots();
  } else {
    test_generator.genNNTaskDAG((NetType)nn);
  }
  task_graphs = test_generator.getTaskGraphs();

  // cout << "correctness: " << test_generator.checkCycleAndConnect(task_graphs[2]) << endl;
  // test_generator.printTaskDAGs();

  // fu_info in processor need set
  // op in FU need set
  // example:
  // Q1. different type of op need different number and speed rate hardware;
  // A1. may solved by set different func: pre's out_size -> in_size
  // Q2. some type of op should shared same hardware, like SLICE and CONCAT
  // A2. ?
  int input_nr = 1;
  int output_nr = 1;
  int conv_nr = 5;
  int fc_nr = 5;
  int pool_nr = 5;
  int active_nr = 5;
  int binary_nr = 5;
  int concat_nr = 5;
  int slice_nr = 5;
  vector<int> fu_nr = {input_nr, output_nr, conv_nr, pool_nr, fc_nr, active_nr, binary_nr, concat_nr, slice_nr};
  for (int op_idx = 0; op_idx < 9; op_idx++) {
    for (auto fu_idx = 0; fu_idx < fu_nr[op_idx]; fu_idx++) {
      FU fu;
      fu.op.type = (OperationType)op_idx;
      processor.fu_info.push_back(fu);
    }
  }
  test_generator.genSpeedTable(processor);
  test_generator.printSpeedTable(processor);
}

void configParams(TaskGraph& tg, Processor& processor) {
  // for (unsigned i = 0; i < tg.tasks.size(); ++i) {
  //   for (unsigned j = 0; j < tg.tasks.size(); ++j) {
  //     if (i == j) { tg.comm_size[i][j] = 0; }
  //     else {
  //       if (tg.comm_size[i][j] != -1) {
  //         tg.comm_size[j][i] = -1;
  //       }
  //     }
  //   }
  // }
  tg.setTaskRelation();
  processor.setOperatorType2FuIdx();
  processor.setAvgCompSpeedForOp();
  processor.setAvgCommSpeedForOp2Op();
}

void partition_schedule_combine_test() {
  std::vector<TaskGraph> task_graphs;
  Processor processor;
  // NetType nn = LENET;
  // NetType nn = ALEXNET;
  // NetType nn = VGG16;
  // NetType nn = GOOGLENET;
  // NetType nn = RESNET18;
  // genTaskGraph(task_graphs, processor, nn);
  genTaskGraph(task_graphs, processor);
  for (TaskGraph& tg : task_graphs) {
    std::cout << "HEFT:" << std::endl;
    configParams(tg, processor);
    // HEFT
    Scheduler heft = Scheduler(tg, processor);
    heft.runHEFT();
    heft.dumpScheduleResult();
    heft.dumpTaskGraph();
    TaskGraph sched_tg_heft = heft.getScheduledTaskGraph();
    Processor sched_p_heft = heft.getScheduledProcessor();

    Metrics m_heft = Metrics(sched_tg_heft, sched_p_heft);
    float speed_up_heft = m_heft.getSppedUp();
    std::cout << "HEFT speed up: " << speed_up_heft << std::endl;

    for (auto& fu : processor.fu_info) {
      fu.clearTaskItem();
    }
    for (auto& task : tg.tasks) {
      task.fu_idx = 0;
      task.start_time = task.finish_time = -1;
    }

    std::cout << "CPOP:" << std::endl;
    Scheduler cpop = Scheduler(tg, processor);
    cpop.runCPOP();
    // cpop.dumpScheduleResult();
    // cpop.dumpTaskGraph();
    TaskGraph sched_tg_cpop = cpop.getScheduledTaskGraph();
    Processor sched_p_cpop = cpop.getScheduledProcessor();

    Metrics m_cpop = Metrics(sched_tg_cpop, sched_p_cpop);
    float speed_up_cpop = m_cpop.getSppedUp();
    std::cout << "CPOP speed up: " << speed_up_cpop << std::endl;

    for (auto& fu : processor.fu_info) {
      fu.clearTaskItem();
    }
    for (auto& task : tg.tasks) {
      task.fu_idx = 0;
      task.start_time = task.finish_time = -1;
    }

    // auto sp_tg = heft.splitTaskByHardwareNum(tg);
    // configParams(sp_tg, processor);
    // // HEFT + split
    // auto sp_heft = Scheduler(sp_tg, processor);
    // sp_heft.runHEFT();
    // TaskGraph sched_tg_sp_heft = sp_heft.getScheduledTaskGraph();
    // Processor sched_p_sp_heft = sp_heft.getScheduledProcessor();

    // Metrics m_sp_heft = Metrics(sched_tg_sp_heft, sched_p_sp_heft);
    // float speed_up_sp_heft = m_sp_heft.getSppedUp();
    // std::cout << "sp HEFT speed up: " << speed_up_sp_heft << std::endl;
    // for (auto& fu : processor.fu_info) {
    //     fu.clearTaskItem();
    // }
    // for (auto& task : sp_tg.tasks) {
    //     task.fu_idx = 0;
    //     task.start_time = task.finish_time = -1;
    // }
    // // CPOP + split
    // auto sp_cpop = Scheduler(sp_tg, processor);
    // sp_cpop.runCPOP();
    // TaskGraph sched_tg_sp_cpop = sp_cpop.getScheduledTaskGraph();
    // Processor sched_p_sp_cpop = sp_cpop.getScheduledProcessor();

    // Metrics m_sp_cpop = Metrics(sched_tg_sp_cpop, sched_p_sp_cpop);
    // float speed_up_sp_cpop = m_sp_cpop.getSppedUp();
    // std::cout << "sp CPOP speed up: " << speed_up_sp_cpop << std::endl;
    // for (auto& fu : processor.fu_info) {
    //     fu.clearTaskItem();
    // }
    // for (auto& task : sp_tg.tasks) {
    //     task.fu_idx = 0;
    //     task.start_time = task.finish_time = -1;
    // }
    break;
  }
}

void iterative_schedule_partition_test() {
  std::vector<TaskGraph> task_graphs;
  Processor processor;
  // NetType nn = LENET;
  // NetType nn = ALEXNET;
  // NetType nn = VGG16;
  // NetType nn = GOOGLENET;
  // NetType nn = RESNET18;
  // genTaskGraph(task_graphs, processor, nn);
  genTaskGraph(task_graphs, processor);
  for (TaskGraph& tg : task_graphs) {
    float min_serial_time = FLT_MAX;
    for (int i = 0; i < 10; ++i) {
      std::cout << "task number: " << tg.tasks.size() << std::endl;
      for (auto &fu : processor.fu_info) {
        fu.clearTaskItem();
      }
      configParams(tg, processor);
      std::cout << "run heft..." << std::endl;
      Scheduler heft = Scheduler(tg, processor);
      heft.runHEFT();
      heft.dumpScheduleResult();
      heft.dumpTaskGraph();
      TaskGraph sched_tg = heft.getScheduledTaskGraph();
      Processor sched_p = heft.getScheduledProcessor();
      if (i == 0) {
        Metrics m = Metrics(tg, processor);
        min_serial_time = m.getMinSerialTime();
      }
      float sched_time = heft.getScheduledTime();
      float speed_up = min_serial_time / sched_time;
      std::cout << "Sched time: " << sched_time << " serial time: " << min_serial_time << " HEFT speed up: " << speed_up << std::endl;
      if (speed_up < 1.01) {
        break;
      }

      std::cout << "run partition..." << std::endl;
      Partition par = Partition(sched_tg, sched_p);
      par.run();
      tg = par.getPartitionTaskGraph();
      processor = par.getProcessor();
    }
    break;
  }
}

int main() {
  // partition_schedule_combine_test();
  iterative_schedule_partition_test();
  return 0;
}

