#ifndef PIPELINESCHEDULE_HEFT_H
#define PIPELINESCHEDULE_HEFT_H

#include <vector>

#include "Processor.h"

class HEFT {
public:
  HEFT(TaskGraph tg, Processor p);

  void run();

private:
  // reckon average computational cost for each task.
  void reckonAvgCompCost();
  // reckon average communicational cost for each task.
  void reckonAvgCommCost();
  // reckon upward rank for each task
  void reckonUpwardRank();

  void sortByUpwardRank();

  void schedule();

  // if src and dst are the same, the value is 0,
  // if src and dst are not connected, the value is -1.
  std::vector<std::vector<float>> avg_comm_cost_;
  std::vector<float> avg_comp_cost_;
  // for task i, suppose task j is the direct successors. k is the exit node.
  // upward_rank(i) = comp_cost(i) + max{comm_cost(i, j) + upward_rank(j)}
  // upward_rand(k) = comp_cost(k)
  std::vector<float> upward_rank_;
  std::vector<unsigned> sorted_task_idx_;

  TaskGraph tg_;
  Processor p_;
  // average communication startup time
  float l_;
};

#endif //PIPELINESCHEDULE_HEFT_H
