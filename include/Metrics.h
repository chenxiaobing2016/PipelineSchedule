#ifndef PIPELINESCHEDULE_METRICS_H
#define PIPELINESCHEDULE_METRICS_H

#include <cfloat>
#include <numeric>
#include <iostream>
#include "Processor.h"

class Metrics {
public:
  Metrics(TaskGraph tg, Processor p);
  float getSppedUp();
  float getMinSerialTime();

private:
  TaskGraph tg_;
  Processor p_;
};

#endif //PIPELINESCHEDULE_METRICS_H
