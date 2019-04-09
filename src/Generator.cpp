#include "Processor.h"
#include "Generator.h"
using namespace std;

void Generator::genRandomTaskDAGs() {
    task_graphs.clear();
    random_device rd;  // Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis_0_5_1_5(0.5, 1.5);
    for (auto graph_idx = 0; graph_idx < task_graph_nr; graph_idx++) {
        int height = (int)(dis_0_5_1_5(gen) * sqrt(v) / alpha + 1);
        int width = (int)(2 * sqrt(v) * alpha + 1);
        genRandomTaskDAG(height, width);
    }
}
void Generator::genRandomTaskDAG(int height, int width) {
  default_random_engine e;
  random_device rd;  // Will be used to obtain a seed for the random number engine
  mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  uniform_real_distribution<> dis_0_25_0_75(0.25, 0.75);
  uniform_real_distribution<> dis_0_0_1_0(0.0, 1.0);
  vector<vector<OpGrid>> DAG(height, vector<OpGrid> (width));
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      float true_width = width * dis_0_25_0_75(gen);
      float threshold = true_width / width;
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int type;
          do {
              float sample = dis_0_0_1_0(gen);
              if (sample < threshold)
                  type = e() % 7 + 3;
              else
                  type = OperationType::EMPTY;
              if (h_idx > 0 || (OperationType)type != OperationType::EMPTY)
                  break;
          } while (true);
          DAG[h_idx][w_idx].ot = (OperationType)type;
      }
  }

  // traverse from top to bottom for link concat and slice using out_degree
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int pre_grids_nr = h_idx * width;
          if (pre_grids_nr == 0)
              break;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::CONCAT) {
              for (auto idx = 0; idx < out_degree; idx++) {
                  int pre_idx = e() % pre_grids_nr;
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
          if (post_grids_nr == 0)
              break;
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::SLICE) {
              for (auto idx = 0; idx < out_degree; idx++) {
                  int suc_idx;
                  suc_idx = e() % post_grids_nr + (h_idx + 1) * width;
                  auto q = &(DAG[suc_idx / width][suc_idx % width]);
                  if(q->ot != OperationType::EMPTY &&
                     find(p->sucs.begin(), p->sucs.end(), suc_idx) == p->sucs.end()) {
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
          if (pre_grids_nr == 0)
              break;
          int pre_idx_1, pre_idx_2;
          bool find_pre = false;
          do {
              pre_idx_1 = e() % pre_grids_nr;
              pre_idx_2 = pre_idx_1 + 1 % pre_grids_nr;
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

  // translate Opgrids to TaskGraph
  vector<TaskNode> tasks;
  TaskNode input;
  input.op.type = OperationType::INPUT;
  input.in_size = input_size;
  input.out_size = input_size;
  tasks.push_back(input);

  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::EMPTY)
              assert(p->pres.empty() && p->sucs.empty());
          if (p->ot != OperationType::EMPTY) {
              p->id = tasks.size();
              TaskNode task;
              task.op.type = p->ot;
              tasks.push_back(task);
          }
      }
  }
  TaskNode output;
  output.op.type = OperationType::OUTPUT;
  tasks.push_back(output);

  int task_nr = tasks.size();
  vector<vector<float>> adj_matrix(task_nr, vector<float> (task_nr, -1.0));
  // link forward
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          auto p = &DAG[h_idx][w_idx];
          if (p->ot != EMPTY) {
              if (h_idx == 0) {
                  adj_matrix[0][p->id] = tasks[0].out_size;
              }
              if (h_idx == height - 1) {
                  adj_matrix[p->id][task_nr - 1] = 1;
              } else {
                  for (auto suc : p->sucs) {
                      auto suc_id = DAG[suc / width][suc % width].id;
                      adj_matrix[p->id][suc_id] = 1;
                  }
              }
          }
      }
  }
  // TODO(pengshaihui): check backward

  // traverse from top to bottom for set size
  for (auto task_idx = 0; task_idx < task_nr; task_idx++) {
      float whole_in_size = 0;
      float whole_out_size = 0;
      for (auto pre_idx = 0; pre_idx <= task_idx; pre_idx++) {
          if (adj_matrix[pre_idx][task_idx] >= 0) {
              whole_in_size += tasks[pre_idx].out_size;
          }
      }
      int sucs_nr = 0;
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
          case OperationType::INPUT:
              whole_out_size = whole_in_size;
              break;
          case OperationType::SLICE:
              for (auto suc_idx = task_idx; suc_idx < task_nr; suc_idx++)
                  if (adj_matrix[task_idx][suc_idx] >= 0)
                      sucs_nr++;
              whole_out_size = whole_in_size / sucs_nr;
              break;
          default:
              assert(0);
              break;
      }
      tasks[task_idx].in_size = whole_in_size;
      tasks[task_idx].in_size = whole_out_size;
  }

  TaskGraph new_task_graph;
  new_task_graph.tasks = tasks;
  new_task_graph.comm_size = adj_matrix;

  task_graphs.push_back(new_task_graph);
}

void Generator::printTaskDAGs() {
    for (auto task_graph_idx = 0; task_graph_idx < task_graphs.size(); task_graph_idx ++) {
        auto tasks = task_graphs[task_graph_idx].tasks;
        auto adj_matrix = task_graphs[task_graph_idx].comm_size;
        cout << "NO. " << task_graph_idx << " task_graph: " << endl;
        for (auto task_idx = 0; task_idx < tasks.size(); task_idx++) {
            string task_info = "{";
            if(task_idx > 0) {
                for (auto pre_idx = 0; pre_idx <= task_idx; pre_idx++) {
                    auto comm_size = adj_matrix[pre_idx][task_idx];
                    if (comm_size >= 0) {
                        task_info += opTypeToName((tasks[pre_idx]).op.type);
                        task_info += to_string(pre_idx);
                        task_info +="(";
                        task_info += to_string(comm_size);
                        task_info += "), ";
                    }
                }
            }
            task_info += "}-->(";
            task_info += to_string(tasks[task_idx].in_size);
            task_info +=")";
            task_info += opTypeToName(tasks[task_idx].op.type);
            task_info += to_string(task_idx);
            task_info +="(";
            task_info += to_string(tasks[task_idx].out_size);
            task_info += ")-->{";
            if (task_idx < tasks.size() - 1) {
                for (auto suc_idx = task_idx; suc_idx < tasks.size(); suc_idx++) {
                    auto comm_size = adj_matrix[task_idx][suc_idx];
                    if (comm_size >= 0) {
                        task_info += opTypeToName((tasks[suc_idx]).op.type);
                        task_info += to_string(suc_idx);
                        task_info +="(";
                        task_info += to_string(comm_size);
                        task_info += "), ";
                    }
                }
            }
            task_info += "}";
            cout << task_info << endl;
            cout << endl;
        }
    }
}

void Generator::genSpeedTable(Processor &processor) {
    processor.bandwith.clear();
    random_device rd;  // Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_real_distribution<> dis_0_5_1_5(0.5, 1.5);
    int op_type_nr = 10;
    vector<float> op_size_vec(op_type_nr, input_size);
    vector<int> op_nr_vec(op_type_nr, 1);
    vector<float> fu_theory_speed_table(op_type_nr, 0);

    for (auto task_graph : task_graphs) {
        for (auto task : task_graph.tasks) {
            op_nr_vec[(unsigned int)task.op.type]++;
            op_size_vec[(unsigned int)task.op.type] += task.in_size;
        }
    }
    for (auto op_idx = 0; op_idx < op_type_nr; op_idx++) {
        float theory_fu_speed = speed_rate * op_size_vec[op_idx] / op_nr_vec[op_idx];
        if (op_idx == 0 || op_idx == op_type_nr - 1)
            theory_fu_speed = FLT_MAX;
        fu_theory_speed_table[op_idx] = theory_fu_speed;
    }

    for (auto fu_idx = 0; fu_idx < processor.fu_info.size(); fu_idx++) {
        auto ot = processor.fu_info[fu_idx].op.type;
        if (ot == INPUT ||
            ot == OUTPUT) {
            processor.fu_info[fu_idx].speed = FLT_MAX;
        } else {
            processor.fu_info[fu_idx].speed = dis_0_5_1_5(gen) * fu_theory_speed_table[(int)ot];
        }
    }

    for (auto src_idx = 0; src_idx < processor.fu_info.size(); src_idx++) {
        vector<float> src_speed_table;
        auto src_thr_speed = fu_theory_speed_table[(int)processor.fu_info[src_idx].op.type];
        for (auto dst_idx = 0; dst_idx < processor.fu_info.size(); dst_idx++) {
            if (src_idx > dst_idx) {
                src_speed_table.push_back(processor.bandwith[dst_idx][src_idx]);
            } else {
                auto dst_thr_speed = fu_theory_speed_table[(int)processor.fu_info[dst_idx].op.type];
                float theory_bandwidth = ccr * (src_thr_speed + dst_thr_speed) / 2;
                float true_bandwidth = dis_0_5_1_5(gen) * theory_bandwidth;
                src_speed_table.push_back(true_bandwidth);
            }
        }
        processor.bandwith.push_back(src_speed_table);
    }
}

void Generator::printSpeedTable(Processor processor) {
    cout << "fu compute speed and bandwidth: " << endl;
    for (auto fu_idx = 0; fu_idx < processor.fu_info.size(); fu_idx++) {
        cout << "fu_" << opTypeToName(processor.fu_info[fu_idx].op.type) << fu_idx;
        cout <<" speed: " << processor.fu_info[fu_idx].speed << endl;
        cout << "bandwidth: ";
        for (auto dst_fu_idx = 0; dst_fu_idx < processor.fu_info.size(); dst_fu_idx++) {
            cout << " " << processor.bandwith[fu_idx][dst_fu_idx] << "\t ";
        }
        cout << endl;
    }
}
std::string Generator::opTypeToName(OperationType ot) {
    switch (ot) {
        case OperationType::INPUT:
            return "INPUT";
        case OperationType::OUTPUT:
            return "OUTPUT";
        case OperationType::EMPTY:
            return "EMPTY";
        case OperationType::BINARY:
            return "BINARY";
        case OperationType::CONV:
            return "CONV";
        case OperationType::POOL:
            return "POOL";
        case OperationType::FC:
            return "FC";
        case OperationType::ACTIVE:
            return "ACTIVE";
        case OperationType::SLICE:
            return "SlICE";
        case OperationType::CONCAT:
            return "CONCAT";
        default:
            return "UNKNOWN";
    }
}
