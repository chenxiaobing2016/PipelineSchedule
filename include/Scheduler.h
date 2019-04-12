#ifndef PIPELINESCHEDULE_SCHEDULER_H
#define PIPELINESCHEDULE_SCHEDULER_H

#include <vector>

#include "Processor.h"

class Scheduler {
public:
  Scheduler(TaskGraph tg, Processor p);

  void runHEFT();

  void dumpScheduleResult(const std::string& file_name = "schedule_result");

  void dumpTaskGraph(const std::string& file_name = "task_info");

private:
  // reckon average computational cost for each task.
  void reckonAvgCompCost();
  // reckon average communicational cost for each task.
  void reckonAvgCommCost();
  // reckon downward rank for each task
  void reckonDownwardRank();
  // reckon upward rank for each task
  void reckonUpwardRank();

  void sortByUpwardRank();

  void sortByUpAndDownwardRank();

  void scheduleHEFT();

  // if src and dst are the same, the value is 0,
  // if src and dst are not connected, the value is -1.
  std::vector<std::vector<float>> avg_comm_cost_;
  std::vector<float> avg_comp_cost_;
  // for task i, suppose task j is the direct successors. k is the exit node.
  // upward_rank(i) = comp_cost(i) + max{comm_cost(i, j) + upward_rank(j)}
  // upward_rand(k) = comp_cost(k)
  std::vector<float> upward_rank_;
  std::vector<float> downward_rank_;
  std::vector<unsigned> HEFT_sorted_task_idx_;
  std::vector<unsigned> CPOP_sorted_task_idx_;

  TaskGraph tg_;
  Processor p_;
  // average communication startup time
  float l_;
};

#endif //PIPELINESCHEDULE_SCHEDULER_H
