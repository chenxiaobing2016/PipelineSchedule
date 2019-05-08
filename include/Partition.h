#ifndef PIPELINESCHEDULE_PARTITION_H
#define PIPELINESCHEDULE_PARTITION_H

#include <stack>
#include "Processor.h"

class Partition {
public:
  Partition(TaskGraph tg, Processor p);

  void run();

  TaskGraph getPartitionTaskGraph();

  Processor getProcessor();

  void taskPartition(unsigned partition_nr, unsigned task_id);

  void tryPartition(std::vector<unsigned> critical_path, unsigned& partition_nr, unsigned& task_id);

  void getCriticalPath(std::vector<unsigned>& critical_path);

  std::vector<unsigned> getFreeFU(OperationType op, float start, float finish);

private:
  TaskGraph tg_;
  Processor p_;
};

#endif //PIPELINESCHEDULE_PARTITION_H
