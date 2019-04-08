#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <float.h>

#include "Processor.h"
#include "Generator.h"
using namespace std;

void Generator::genRandomTaskDAG() {
  tasks.clear();
  default_random_engine e;
  vector<vector<OpGrid>> DAG(height, vector<OpGrid> (width));
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int type;
          do {
              type = e() % 8 + 2;
          } while (h_idx > 0 || (OperationType)type != OperationType::EMPTY);
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
                  int pre_idx = (pre_grids_nr > 0) ? (e() % pre_grids_nr) : 0;
                  auto q = &(DAG[pre_idx / width][pre_idx % width]);
                  if(q->ot != OperationType::EMPTY &&
                  find(p->pres.begin(), p->pres.end(), pre_idx) == p->pres.end()) {
                      p->pres.push_back(pre_idx);
                      q->sucs.push_back(h_idx * width + w_idx);
                  }
              }
          }
      }
  }
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int post_grids_nr = (height - 1 - h_idx) * width;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::SLICE) {
              for (auto idx = 0; idx < out_degree; idx++) {
                  int suc_idx;
                  suc_idx = (post_grids_nr > 0) ? (e() % post_grids_nr + (h_idx + 1) * width) : 0;
                  auto q = &(DAG[suc_idx / width][suc_idx % width]);
                  if(q->ot != OperationType::EMPTY &&
                     find(p->sucs.begin(), p->sucs.end(), suc_idx) == p->sucs.end() &&
                     ) {
                      bool available = false;
                      switch(q->ot) {
                          case OperationType::CONV:
                          case OperationType::ACTIVE:
                          case OperationType::POOL:
                          case OperationType::FC:
                          case OperationType::SLICE:
                              if (q->pres.empty())
                                  available = true;
                              break;
                          case OperationType::BINARY:
                              if (q->pres.size() < 2)
                                  available = true;
                              break;
                          case OperationType::CONCAT:
                              available = true;
                              break;
                          default:
                              break;
                      }
                      if (available) {
                          p->sucs.push_back(suc_idx);
                          q->pres.push_back(h_idx * width + w_idx);
                      }
                  }
              }
          }
      }
  }

  // traverse link other layers
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int pre_grids_nr = h_idx * width;
          int pre_idx_1, pre_idx_2;
          bool find_pre = false;
          do {
              pre_idx_1 = (pre_grids_nr > 0) ? (e() % pre_grids_nr) : 0;
              pre_idx_2 = (pre_grids_nr > 0) ? (pre_idx_1 + 1 % pre_grids_nr) : 0;
              if (DAG[pre_idx_1 / width][pre_idx_1 % width].ot != OperationType::EMPTY &&
                  DAG[pre_idx_2 / width][pre_idx_2 % width].ot != OperationType::EMPTY)
                  find_pre = true;
          } while (pre_grids_nr > 0 || !find_pre);
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::CONV ||
              p->ot == OperationType::POOL ||
              p->ot == OperationType::FC ||
              p->ot == OperationType::ACTIVE) {
              if (!p->pres.empty()) {
                  continue;
              } else {
                  p->pres.push_back(pre_idx_1);
                  int q_idx = p->pres[0];
                  auto q = &(DAG[q_idx / width][q_idx % width]);
                  q->pres.push_back(h_idx * width + w_idx);
              }
          } else if (p->ot ==OperationType::BINARY) {
              if (p->pres.size() > 1) {
                  continue;
              } else {
                  if (p->pres.empty()) {
                      p->pres.push_back(pre_idx_1);
                      int q_idx = p->pres[0];
                      auto q = &(DAG[q_idx / width][q_idx % width]);
                      q->pres.push_back(h_idx * width + w_idx);
                  }
                  p->pres.push_back(pre_idx_2);
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

  // remove empty task
  auto task_iter = tasks.begin();
  while (task_iter != tasks.end()) {
      if ((*task_iter).op.type == OperationType::EMPTY) {
          task_iter = tasks.erase(task_iter);
      } else {
          if ((*task_iter).op.type != OperationType::OUTPUT &&
          (*task_iter).successors.empty()) {
              (*task_iter).successors.push_back(&tasks.back());
          }
          task_iter++;
      }
  }

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
              whole_out_size = 0;
              break;
      }
      tasks[task_idx].in_size = whole_out_size;
  }
}

void Generator::printTaskDAG() {}

void Generator::genSpeedTable() {
    fu_speed_table.clear();
    comm_speed_table.clear();
    random_device rd;  // Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis1(0.5, 1.5);
    int op_type_nr = 9;
    vector<float> op_size_vec(op_type_nr, input_size);
    vector<int> op_nr_vec(op_type_nr, 1);

    for (auto &task_item : tasks) {
        op_nr_vec[(unsigned int)task_item.op.type]++;
        op_size_vec[(unsigned int)task_item.op.type] += task_item.in_size;
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

void Generator::printSpeedTable() {}