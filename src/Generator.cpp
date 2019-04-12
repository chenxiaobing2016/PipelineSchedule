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
  random_device rd;  // Will be used to obtain a seed for the random number engine
  mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  uniform_real_distribution<> dis_0_25_0_75(0.25, 0.75);
  uniform_real_distribution<> dis_0_0_1_0(0.0, 1.0);
  uniform_int_distribution<>  op_type(3, 9);
  uniform_int_distribution<>  position(0, height * width);
  vector<vector<OpGrid>> DAG(height, vector<OpGrid> (width));
  for (auto h_idx = 0; h_idx < height; h_idx++) {
      float true_width = width * dis_0_25_0_75(gen);
      float threshold = true_width / width;
      for (auto w_idx = 0; w_idx < width; w_idx++) {
          int type;
          do {
              float sample = dis_0_0_1_0(gen);
              if (sample < threshold)
                  type = op_type(gen) % 7 + 2;
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
                  int pre_idx;
                  do {
                      pre_idx = position(gen) % pre_grids_nr;
                  } while (DAG[pre_idx / width][pre_idx % width].ot == EMPTY);
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
                  suc_idx = position(gen) % post_grids_nr + (h_idx + 1) * width;
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
              pre_idx_1 = position(gen) % pre_grids_nr;
              pre_idx_2 = position(gen) % pre_grids_nr;
              if (DAG[pre_idx_1 / width][pre_idx_1 % width].ot != OperationType::EMPTY &&
                  DAG[pre_idx_2 / width][pre_idx_2 % width].ot != OperationType::EMPTY)
                  find_pre = true;
          } while (!find_pre);
          auto p = &DAG[h_idx][w_idx];
          if (p->ot == OperationType::CONV ||
              p->ot == OperationType::POOL ||
              p->ot == OperationType::FC ||
              p->ot == OperationType::ACTIVE ||
              p->ot == OperationType::SLICE) {
              if (!p->pres.empty()) {
                  continue;
              } else {
                  p->pres.push_back(pre_idx_1);
                  int q_idx = p->pres[0];
                  auto q = &(DAG[q_idx / width][q_idx % width]);
                  q->sucs.push_back(h_idx * width + w_idx);
              }
          } else if (p->ot ==OperationType::BINARY) {
              if (p->pres.size() > 1) {
                  continue;
              } else {
                  if (p->pres.empty()) {
                      p->pres.push_back(pre_idx_1);
                      auto q = &(DAG[pre_idx_1 / width][pre_idx_1 % width]);
                      q->sucs.push_back(h_idx * width + w_idx);
                  }
                  p->pres.push_back(pre_idx_2);
                  auto q = &(DAG[pre_idx_2 / width][pre_idx_2 % width]);
                  q->sucs.push_back(h_idx * width + w_idx);
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

  int no_sucs_layer = 0;
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
              if (p->sucs.empty())
                  no_sucs_layer++;
          }
      }
  }

  // add some concat for no sucs layers
  for (int add_concat_idx = 0; add_concat_idx < no_sucs_layer / out_degree; add_concat_idx++) {
      TaskNode concat;
      concat.op.type = CONCAT;
      tasks.push_back(concat);
  }

  // add output at last
  TaskNode output;
  output.op.type = OperationType::OUTPUT;
  tasks.push_back(output);

  int task_nr = tasks.size();

  // prepare for no sucs layers
  vector<int> concat_vec;
  for (auto task_idx = 0; task_idx < task_nr; task_idx++) {
      if (tasks[task_idx].op.type == CONCAT ||
          tasks[task_idx].op.type == OUTPUT) {
          concat_vec.push_back(task_idx);
      }
  }
  uniform_int_distribution<>  concat_pos(0, concat_vec.size());

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
                  if (p->sucs.empty()) {
                      int start = 0;
                      for (start = 0; start < concat_vec.size(); start++) {
                          if (concat_vec[start] > p->id)
                              break;
                      }
                      int offset = concat_pos(gen) % (concat_vec.size() - start);
                      assert(start + offset < concat_vec.size());
                      adj_matrix[p->id][concat_vec[start + offset]] = 1;
                  }
              }
          }
      }
  }

  // check no pres or no sucs layers
  vector<bool> has_pres(task_nr, false);
  vector<bool> has_sucs(task_nr, false);
  has_pres[0] = true;
  has_sucs[task_nr - 1] = true;
  for (auto src_idx = 0; src_idx < task_nr; src_idx++) {
      for (auto dst_idx = 0; dst_idx < task_nr; dst_idx++) {
          if (adj_matrix[src_idx][dst_idx] >= 0) {
              has_pres[dst_idx] = true;
              has_sucs[src_idx] = true;
          }
      }
  }
  for (auto task_idx = 0; task_idx < task_nr; task_idx++) {
      if (!has_pres[task_idx]) {
          assert(tasks[task_idx].op.type == CONCAT);
          adj_matrix[0][task_idx] = input_size;
      }
      if (!has_sucs[task_idx]) {
          assert(tasks[task_idx].op.type == CONCAT);
          int start = 0;
          for (start = 0; start < concat_vec.size(); start++) {
              if (concat_vec[start] > task_idx)
                  break;
          }
          int offset = concat_pos(gen) % (concat_vec.size() - start);
          assert(start + offset < concat_vec.size());
          adj_matrix[task_idx][concat_vec[start + offset]] = 1;
      }
  }
  // TODO(pengshaohui): delete one in one out concat and slice

  for (auto task_iter = tasks.begin(); task_iter != tasks.end(); task_iter++) {
      auto task = *task_iter;
      int task_idx = task_iter - tasks.begin();
      if (task.op.type == CONCAT || task.op.type == SLICE) {
          int pre_nr = 0;
          int suc_nr = 0;
          int pre_task_idx = -1;
          int suc_task_idx = -1;
          for (auto pre_idx = 0; pre_idx < tasks.size(); pre_idx++) {
              if (adj_matrix[pre_idx][task_idx] >= 0) {
                  pre_nr++;
                  pre_task_idx = pre_idx;
              }
          }
          for (auto suc_idx = 0; suc_idx < tasks.size(); suc_idx++) {
              if (adj_matrix[task_idx][suc_idx] >= 0) {
                  suc_nr++;
                  suc_task_idx = suc_idx;
              }
          }
          if (pre_nr == 1 && suc_nr == 1) {
              if (tasks[pre_task_idx].op.type == INPUT)
                  adj_matrix[pre_task_idx][suc_task_idx] = input_size;
              else
                  adj_matrix[pre_task_idx][suc_task_idx] = 1;
              task_iter = tasks.erase(task_iter);
              for (auto links : adj_matrix) {
                  links.erase(links.begin() + task_idx);
              }
              adj_matrix.erase(adj_matrix.begin() + task_idx);
          }
      }
  }
  // TODO(pengshaihui): check backward

  // traverse from top to bottom for set size
  // for (auto x : adj_matrix) {
  //     for (auto y : x) {
  //         cout << y << "\t ";
  //     }
  //     cout << endl;
  // }
  for (auto task_idx = 1; task_idx < task_nr; task_idx++) {
      float whole_in_size = 0;
      float whole_out_size = 0;
      float comp_size;
      for (auto pre_idx = 0; pre_idx <= task_idx; pre_idx++) {
          if (adj_matrix[pre_idx][task_idx] >= 0) {
              adj_matrix[pre_idx][task_idx] = tasks[pre_idx].out_size;
              whole_in_size += tasks[pre_idx].out_size;
          }
      }
      int sucs_nr = 0;
      switch(tasks[task_idx].op.type) {
          case OperationType::CONV:
          case OperationType::FC:
              whole_out_size = whole_in_size * out_rate;
              comp_size = 10 * whole_in_size;
              break;
          case OperationType::POOL:
              whole_out_size = whole_in_size * out_rate;
              comp_size = whole_in_size;
              break;
          case OperationType::CONCAT:
              whole_out_size = whole_in_size;
              comp_size = pow(whole_in_size, 0.5);
              break;
          case OperationType::ACTIVE:
          case OperationType::BINARY:
          case OperationType::OUTPUT:
          case OperationType::INPUT:
              whole_out_size = whole_in_size;
              whole_out_size = whole_in_size;
              comp_size = whole_in_size;
              break;
          case OperationType::SLICE:
              for (auto suc_idx = task_idx; suc_idx < task_nr; suc_idx++)
                  if (adj_matrix[task_idx][suc_idx] >= 0)
                      sucs_nr++;
              whole_out_size = whole_in_size / sucs_nr;
              comp_size = pow(whole_in_size, 0.5);
              break;
          default:
              assert(0);
              break;
      }
      tasks[task_idx].in_size = whole_in_size;
      tasks[task_idx].out_size = whole_out_size;
      tasks[task_idx].comp_size = comp_size;
  }

  TaskGraph new_task_graph;
  new_task_graph.tasks = tasks;
  new_task_graph.comm_size = adj_matrix;

  task_graphs.push_back(new_task_graph);
}

bool Generator::checkCycleAndConnect(TaskGraph TG) {
    vector<bool> visited(TG.tasks.size(), false);
    function <bool(int, int)> recVis = [&](int task_idx, int layer)->bool {
        if (layer > TG.tasks.size())
            return false;
        for (auto dst_idx = 0; dst_idx < TG.tasks.size(); dst_idx++) {
            if (TG.comm_size[task_idx][dst_idx] >= 0 && !visited[dst_idx]) {
                visited[dst_idx] = true;
                if (!recVis(dst_idx, layer + 1))
                    return false;
            }
        }
        return true;
    };
    return recVis(0, 0);
}

vector<TaskGraph> Generator::getTaskGraphs() {
    return task_graphs;
}

void Generator::printTaskDAGs() {
    for (auto task_graph_idx = 0; task_graph_idx < task_graphs.size(); task_graph_idx ++) {
        auto tasks = task_graphs[task_graph_idx].tasks;
        auto adj_matrix = task_graphs[task_graph_idx].comm_size;
        cout << "NO. " << task_graph_idx << " task_graph: " << endl;
        for (auto task_idx = 0; task_idx < tasks.size(); task_idx++) {
            string task_info = "{";
            if(task_idx > 0) {
                bool has_pres = false;
                for (auto pre_idx = 0; pre_idx <= task_idx; pre_idx++) {
                    auto comm_size = adj_matrix[pre_idx][task_idx];
                    if (comm_size >= 0) {
                        has_pres = true;
                        task_info += opTypeToName((tasks[pre_idx]).op.type);
                        task_info += to_string(pre_idx);
                        task_info +="(";
                        task_info += to_string(comm_size);
                        task_info += "), ";
                    }
                }
                assert(has_pres);
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
                bool has_sucs = false;
                for (auto suc_idx = task_idx; suc_idx < tasks.size(); suc_idx++) {
                    auto comm_size = adj_matrix[task_idx][suc_idx];
                    if (comm_size >= 0) {
                        has_sucs = true;
                        task_info += opTypeToName((tasks[suc_idx]).op.type);
                        task_info += to_string(suc_idx);
                        task_info +="(";
                        task_info += to_string(comm_size);
                        task_info += "), ";
                    }
                }
                assert(has_sucs);
            }
            task_info += "}";
            cout << task_info << endl;
            cout << endl;
        }
    }
}

void Generator::genDAGdots() {
    for (auto tg_idx = 0; tg_idx < task_graphs.size(); tg_idx++) {
        string file_name = "task_graph" + to_string(tg_idx) + ".dot";
        ofstream fout(file_name);
        fout << "digraph tg" << to_string(tg_idx) << "{\n";
        fout << "\tsize = \"4,4\";\n";
        vector<string> task_labels;
        vector<string> task_names;
        for (auto task_idx = 0; task_idx < task_graphs[tg_idx].tasks.size(); task_idx++) {
            auto task = task_graphs[tg_idx].tasks[task_idx];
            string name = opTypeToName(task.op.type) + to_string(task_idx);
            string in_size = "\\nin_size: " + to_string(task.in_size) + "\\n";
            string fu_idx = "fu_idx: " + to_string(task.fu_idx) + "\\n";
            string s_t = "start: " + to_string(task.start_time) + "\\n";
            string f_t = "finish: " + to_string(task.finish_time);
            string task_label = name + in_size + fu_idx + s_t + f_t;
            task_labels.push_back(task_label);
            task_names.push_back(name);
            fout << "\t" << name << " [label=\"" << task_label << "\"];\n";
        }
        for (auto task_idx = 1; task_idx < task_graphs[tg_idx].tasks.size(); task_idx++) {
            for (auto prec_idx = 0; prec_idx <= task_idx; prec_idx++) {
                auto comm_size = task_graphs[tg_idx].comm_size[prec_idx][task_idx];
                if (comm_size >= 0) {
                    string link;
                    link = task_names[prec_idx] + " -> " + task_names[task_idx];
                    fout << "\t" << link << " [label=\"" << to_string(comm_size) << "\"];\n";
                }
            }
        }
        fout << "}";
        fout.close();
    }
}

void Generator::genSpeedTable(Processor &processor) {
    processor.bandwidth.clear();
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
    assert(op_nr_vec.back() == 1);
    for (auto op_idx = 0; op_idx < op_type_nr; op_idx++) {
        float theory_fu_speed = speed_rate * op_size_vec[op_idx] / op_nr_vec[op_idx];
        if (op_idx == (int)INPUT || op_idx == (int)OUTPUT)
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
                src_speed_table.push_back(processor.bandwidth[dst_idx][src_idx]);
            } else {
                auto dst_thr_speed = fu_theory_speed_table[(int)processor.fu_info[dst_idx].op.type];
                float theory_bandwidth = ccr * (src_thr_speed + dst_thr_speed) / 2;
                float true_bandwidth = dis_0_5_1_5(gen) * theory_bandwidth;
                src_speed_table.push_back(true_bandwidth);
            }
        }
        processor.bandwidth.push_back(src_speed_table);
    }
}

void Generator::printSpeedTable(Processor processor) {
    cout << "fu compute speed and bandwidth: " << endl;
    for (auto fu_idx = 0; fu_idx < processor.fu_info.size(); fu_idx++) {
        cout << "fu_" << opTypeToName(processor.fu_info[fu_idx].op.type) << fu_idx;
        cout <<" speed: " << processor.fu_info[fu_idx].speed << endl;
        cout << "bandwidth: ";
        for (auto dst_fu_idx = 0; dst_fu_idx < processor.fu_info.size(); dst_fu_idx++) {
            cout << dst_fu_idx << "->" << processor.bandwidth[fu_idx][dst_fu_idx] << " ";
        }
        cout << "\n" << endl;
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
