#!/usr/bin/env python3

import os

def read_task_info(task_info_file):
  task_info = []
  relation_info = []
  with open(task_info_file) as f:
    for line in f:
      elems = line.split()
      task_id = int(elems[0])
      operation_type = elems[1]
      fu_id = int(elems[2])
      comp_size = float(elems[3])
      comp_speed = float(elems[4])
      start_time = float(elems[5])
      end_time = float(elems[6])
      task_info.append([task_id, operation_type, fu_id, comp_size, comp_speed, start_time, end_time])
      idx = 7
      task_relation = []
      while idx < len(elems) - 1:
        succ_id = int(elems[idx])
        comm_size = float(elems[idx + 1])
        comm_speed = float(elems[idx + 2])
        comm_time = float(elems[idx + 3])
        task_relation.append([succ_id, comm_size, comm_speed, comm_time])
        idx += 4
      relation_info.append(task_relation)
  return task_info, relation_info

def write_dot(task_info, relation_info, file_name):
  with open(file_name, 'w') as f:
    f.write('digraph TaskGraph {\n')
    for item in task_info:
      # label = 'Task id: {} type: {} fu id: {} size: {} speed: {} start: {} end: {}'.format(item[0], item[1], item[2], item[3], item[4], item[5], item[6])
      label = 'Task id: {} type: {} time: {}'.format(item[0], item[1], item[3] / item[4])
      f.write('{} [label=\"{}\"]\n'.format(item[0], label))
    for task_id in range(len(relation_info)):
      for succ in relation_info[task_id]:
        label = 'size: {} speed: {} time: {}'.format(succ[1], succ[2], succ[3])
        f.write('{} -> {} [label=\"{}\"]\n'.format(task_id, succ[0], label))
    f.write('}')

if __name__ == '__main__':
  task_info, relation_info = read_task_info('../build/task_info')
  write_dot(task_info, relation_info, 'task_info.dot')
  cmd = "dot -Tpng {0}.dot -o {0}.png".format('task_info')
  os.system(cmd)
