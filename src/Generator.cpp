#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <float.h>

#include "Processor.h"
#include "Generator.h"
using namespace std;

/*
void Generator::genRandomTaskDAG() {
  tasks.clear();
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


  // translate Opgrids to tasks
  Task input;
  input.op.type = OperationType::INPUT;
  input.in_size = input_size;
  input.out_size = input_size;
  tasks.push_back(input);
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          auto p = &DAG[h_idx][w_idx];
          Task task;
          task.op.type = p->ot;
          for (auto in_idx = 0; in_idx < p->pres.size(); in_idx++) {
              if(h_idx == 0) {
                  task.precursors.push_back(&tasks[0]);
                  tasks[0].successors.push_back(&task);
              } else {
                  task.precursors.push_back(&tasks[p->pres[in_idx] + 1]);
                  tasks[p->pres[in_idx] + 1].successors.push_back(&task);
              }
          }
          tasks.push_back(task);
      }
  }
  Task output;
  output.op.type = OperationType::OUTPUT;
  for (auto w_idx = 0; w_idx < width; w_idx++) {
      output.precursors.push_back(&tasks[tasks.size() - (w_idx + 1)]);
      tasks[tasks.size() - (w_idx + 1)].successors.push_back(&output);
  }
  tasks.push_back(output);

  // traverse from top to bottom for set size
  for (auto task_idx = 1; task_idx < tasks.size(); task_idx++) {
      float whole_in_size = 0;
      for (auto in_iter : tasks[task_idx].precursors) {
          whole_in_size += in_iter->out_size;
      }
      tasks[task_idx].in_size = whole_in_size;
      float whole_out_size;
      switch(tasks[task_idx].op.type) {
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
              whole_out_size = whole_in_size / tasks[task_idx].successors.size();
              break;
          default:
              break;
      }
  }
}

void Generator::genSpeedTable() {
    fu_speed_table.clear();
    comm_speed_table.clear();
    random_device rd;  // Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis1(0.5, 1.5);
    int op_type_nr = 9;
    vector<float> op_size_vec(op_type_nr, input_size);
    vector<int> op_nr_vec(op_type_nr, 1);

    for (auto task_idx = 0; task_idx < tasks.size(); task_idx++) {
        op_nr_vec[(unsigned int)(tasks[task_idx].op.type)] ++;
        op_size_vec[(unsigned int)(tasks[task_idx].op.type)] += tasks[task_idx].in_size;
    }

    for (auto op_idx = 0; op_idx < op_type_nr; op_idx++) {
        float theory_fu_speed = speed_rate * op_size_vec[op_idx] / op_nr_vec[op_idx];
        float true_speed = dis1(gen) * theory_fu_speed;
        if (op_idx == 0 || op_idx == op_type_nr - 1)
            true_speed = FLT_MAX;
        fu_speed_table.push_back(true_speed);
    }

    for (auto op_idx_i = 0; op_idx_i < op_type_nr; op_idx_i++) {
        vector<float> src_i;
        float src_thr_fu_speed = speed_rate * op_size_vec[op_idx_i] / op_nr_vec[op_idx_i];
        for (auto op_idx_j = 0; op_idx_j < op_type_nr; op_idx_j++) {
            float dst_thr_fu_speed = speed_rate * op_size_vec[op_idx_j] / op_nr_vec[op_idx_j];
            float theory_comm_speed = ccr * (src_thr_fu_speed + dst_thr_fu_speed) / 2;
            float true_comm_speed = dis1(gen) * theory_comm_speed;
            src_i.push_back(true_comm_speed);
        }
        comm_speed_table.push_back(src_i);
    }
}
 */
