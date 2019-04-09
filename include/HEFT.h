#ifndef PIPELINESCHEDULE_HEFT_H
#define PIPELINESCHEDULE_HEFT_H

#include <vector>

#include "Processor.h"

class HEFT {
public:
  HEFT(TaskGraph tg, Processor p);

  void run();

private:
  void reckonAvgCompCost();

  void reckonAvgCommCost();

  void reckonUpwardRank();

  void sortByUpwardRank();

  void schedule();

  std::vector<std::vector<float>> avg_comm_cost_;
  std::vector<float> avg_comp_cost_;
  std::vector<float> upward_rank_;
  std::vector<unsigned> sorted_task_idx_;

  TaskGraph tg_;
  Processor p_;
  // average communication startup time
  float l_;
};

#endif //PIPELINESCHEDULE_HEFT_H
