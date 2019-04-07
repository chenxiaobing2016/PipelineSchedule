#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include "Processor.h"
#include "Generator.h"
using namespace std;

vector<Task> Generator::genTaskDAG() {
  default_random_engine e;
  vector<vector<OpGrid>> DAG(height, vector<OpGrid> (width));
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int type = e() % 7 + 2;
          DAG[h_idx][w_idx].ot = (OperationType)type;
      }
    }

  // traverse from top to bottom for link concat and slice using out_degree
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int pre_grids_nr = h_idx * width;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::CONCAT) {
              for (auto idx = 0; idx < out_degree; idx++) {
                  p->pres.push_back(e() % pre_grids_nr);
              }
              sort(p->pres.begin(), p->pres.end());
              p->pres.erase(unique(p->pres.begin(), p->pres.end()), p->pres.end());
              for (auto idx = 0; idx < p->pres.size(); idx++) {
                  int q_idx = p->pres[idx];
                  auto q = &(DAG[q_idx / width][q_idx % w_idx]);
                  q->sucs.push_back(h_idx * width + w_idx);
              }
          }
      }
  }
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int post_grids_nr = h_idx * width;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::SLICE) {
              for (auto idx = 0; idx < out_degree; idx++) {
                  p->sucs.push_back(e() % post_grids_nr);
              }
              sort(p->sucs.begin(), p->pres.end());
              p->sucs.erase(unique(p->sucs.begin(), p->sucs.end()), p->sucs.end());
              for (auto idx = 0; idx < p->sucs.size(); idx++) {
                  int q_idx = p->sucs[idx];
                  auto q = &(DAG[q_idx / width][q_idx % w_idx]);
                  q->pres.push_back(h_idx * width + w_idx);
              }
          }
      }
  }

  // traverse link other layers
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int pre_grids_nr = h_idx * width;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::CONV ||
              p->ot == OperationType::POOL ||
              p->ot == OperationType::FC ||
              p->ot == OperationType::ACTIVE) {
              if (p->pres.size() > 0) {
                  continue;
              }
              else {
                  p->pres.push_back(e() % pre_grids_nr);
                  int q_idx = p->pres[0];
                  auto q = &(DAG[q_idx / width][q_idx % w_idx]);
                  q->pres.push_back(h_idx * width + w_idx);
              }
          } else if (p->ot ==OperationType::BINARY) {
              if (p->pres.size() > 1) {
                  continue;
              } else {
                  if (p->pres.size() == 0) {
                      p->pres.push_back(e() % pre_grids_nr);
                      int q_idx = p->pres[0];
                      auto q = &(DAG[q_idx / width][q_idx % w_idx]);
                      q->pres.push_back(h_idx * width + w_idx);
                  }
                  int id = p->pres[0];
                  p->pres.push_back(id + 1 % pre_grids_nr);
              }
          }
      }
  }


  // translate Opgrids to Tasks
  vector<Task> Tasks;
  Task input;
  input.op.type = OperationType::INPUT;
  input.in_size = input_size;
  input.out_size = input_size;
  Tasks.push_back(input);
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          auto p = &DAG[h_idx][w_idx];
          Task task;
          task.op.type = p->ot;
          for (auto in_idx = 0; in_idx < p->pres.size(); in_idx++) {
              if(h_idx == 0) {
                  task.precursors.push_back(&Tasks[0]);
                  Tasks[0].successors.push_back(&task);
              } else {
                  task.precursors.push_back(&Tasks[p->pres[in_idx] + 1]);
                  Tasks[p->pres[in_idx] + 1].successors.push_back(&task);
              }
          }
          Tasks.push_back(task);
      }
  }
  Task output;
  output.op.type = OperationType::OUTPUT;
  for (auto w_idx = 0; w_idx < width; w_idx++) {
      output.precursors.push_back(&Tasks[Tasks.size() - (w_idx + 1)]);
      Tasks[Tasks.size() - (w_idx + 1)].successors.push_back(&output);
  }
  Tasks.push_back(output);

  // traverse from top to bottom for set size
  for (auto task_idx = 1; task_idx < Tasks.size(); task_idx++) {
      float whole_in_size = 0;
      for (auto in_iter : Tasks[task_idx].precursors) {
          whole_in_size += in_iter->out_size;
      }
      Tasks[task_idx].in_size = whole_in_size;
      float whole_out_size;
      switch(Tasks[task_idx].op.type) {
          case OperationType::CONV:
          case OperationType::FC:
          case OperationType::POOL:
              whole_out_size = whole_in_size * out_rate;
              break;
          case OperationType::CONCAT:
          case OperationType::ACTIVE:
          case OperationType::BINARY:
          case OperationType::OUTPUT:
              whole_out_size = whole_in_size;
              break;
          case OperationType::SLICE:
              whole_out_size = whole_in_size / Tasks[task_idx].successors.size();
              break;
          default:
              break;
      }
  }
  return Tasks;
}
