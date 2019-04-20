#include <Metrics.h>

Metrics::Metrics(TaskGraph tg, Processor p)
    : tg_(tg), p_(p) {}

float Metrics::getSppedUp() {
  float min_serial_time = getMinSerialTime();
  float schedule_time = 0;
  for (unsigned task_id = 0u; task_id < tg_.tasks.size(); ++task_id) {
    schedule_time = std::max(schedule_time, tg_.tasks[task_id].finish_time);
  }
  return min_serial_time / schedule_time;
}

float Metrics::getMinSerialTime() {
  std::unordered_map<OperationType, float> op_comp_size;
  std::unordered_map<std::pair<OperationType, OperationType>, float> op2op_comm_size;
  for (unsigned task_idx = 0u; task_idx < tg_.tasks.size(); ++task_idx) {
    OperationType opt = tg_.tasks[task_idx].op.type;
    float size = tg_.tasks[task_idx].comp_size;
    if (op_comp_size.count(opt) == 0) {
      op_comp_size[opt] = size;
    } else {
      op_comp_size[opt] += size;
    }
  }
  for (unsigned src_idx = 0u; src_idx < tg_.tasks.size(); ++src_idx) {
    for (unsigned dst_idx = 0u; dst_idx < tg_.tasks.size(); ++dst_idx) {
      if (src_idx != dst_idx && tg_.comm_size[src_idx][dst_idx] > 0) {
        OperationType src_opt = tg_.tasks[src_idx].op.type;
        OperationType dst_opt = tg_.tasks[dst_idx].op.type;
        if (src_opt != dst_opt) {
          auto idx = std::make_pair(src_opt, dst_opt);
          if (op2op_comm_size.count(idx) == 0) {
            op2op_comm_size[idx] = tg_.comm_size[src_idx][dst_idx];
          } else {
            op2op_comm_size[idx] += tg_.comm_size[src_idx][dst_idx];
          }
        }
      }
    }
  }
  std::unordered_map<OperationType, unsigned> opt_base;
  std::unordered_map<OperationType, unsigned> opt_idx;
  std::unordered_map<OperationType, unsigned> opt_size;
  unsigned size = 1;
  for (auto i : p_.opt_fu_idx) {
    if (op_comp_size.count(i.first) != 0) {
      opt_base[i.first] = size;
      opt_idx[i.first] = 0;
      opt_size[i.first] = i.second.size();
      size *= opt_size[i.first];
    }
  }

  float min_time = FLT_MAX;
  std::unordered_map<OperationType, unsigned> min_opt;
  for (unsigned case_idx = 0; case_idx < size; ++case_idx) {
    for (auto i : opt_idx) {
      opt_idx[i.first] = (case_idx / opt_base[i.first]) % opt_size[i.first];
    }
    float tmp_time = 0;
    for (auto item : op_comp_size) {
      unsigned fu_idx = p_.opt_fu_idx[item.first][opt_idx[item.first]];
      tmp_time += item.second / p_.fu_info[fu_idx].speed;
    }
    for (auto item : op2op_comm_size) {
      OperationType src_type = item.first.first;
      OperationType dst_type = item.first.second;
      unsigned src_fu_idx = p_.opt_fu_idx[src_type][opt_idx[src_type]];
      unsigned dst_fu_idx = p_.opt_fu_idx[dst_type][opt_idx[dst_type]];
      auto idx = std::make_pair(src_type, dst_type);
      tmp_time += op2op_comm_size[idx] / p_.bandwidth[src_fu_idx][dst_fu_idx];
    }
    if (tmp_time < min_time) {
      min_time = tmp_time;
      min_opt.clear();
      for (auto i : opt_idx) {
        min_opt[i.first] = p_.opt_fu_idx[i.first][opt_idx[i.first]];
      }
    }
  }
#if 0
  std::cout << "Fu sum: " << p_.fu_info.size() << std::endl;
  std::cout << "Min time: " << min_time << std::endl;
  std::cout << "Fu config:" << std::endl;
  for (auto i : min_opt) {
    std::cout << "Operation Type: " << toString(i.first)
              << " Fu in: " << i.second
              << std::endl;
  }
#endif

  return min_time;
 }
