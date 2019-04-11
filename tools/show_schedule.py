#!/usr/bin/env python

import matplotlib.pyplot as plt

def get_input(in_file):
  task_info = []
  with open(in_file) as f:
    for line in f:
      task_info.append([])
      tasks = line.split()
      idx = 0
      while idx < len(tasks):
        fu_idx = int(tasks[idx])
        start_time = float(tasks[idx + 1])
        finish_time = float(tasks[idx + 2])
        idx += 3
        task_info[-1].append([fu_idx, start_time, finish_time])
  return task_info

def get_max_time(task_info):
  max_time = 0
  for p_idx in range(len(task_info)):
    for t_idx in range(len(task_info[p_idx])):
      max_time = max(max_time, task_info[p_idx][t_idx][2])
  return max_time

def rearange_task(task_info, max_time):
  ret_val = []
  for p_idx in range(len(task_info)):
    ret_val.append([])
    for t_idx in range(len(task_info[p_idx])):
      if t_idx == 0:
        if task_info[p_idx][t_idx][1] != 0:
          ret_val[-1].append([-1, 0, task_info[p_idx][t_idx][1]])
        ret_val[-1].append(task_info[p_idx][t_idx])
      else:
        if task_info[p_idx][t_idx][1] != task_info[p_idx][t_idx - 1][2]:
          ret_val[-1].append([-1, task_info[p_idx][t_idx - 1][2], task_info[p_idx][t_idx][1]])
        ret_val[-1].append(task_info[p_idx][t_idx])
    if len(task_info[p_idx]) == 0:
      ret_val[-1].append([-1, 0, max_time])
    else:
      ret_val[-1].append([-1, task_info[p_idx][t_idx][2], max_time])
  return ret_val

def print_processor_usage(task_info):
  has_idle_label = False
  for p_idx in range(len(task_info)):
    y = p_idx + 1
    for t_idx in range(len(task_info[p_idx])):
      task_id = task_info[p_idx][t_idx][0]
      start   = task_info[p_idx][t_idx][1]
      end     = task_info[p_idx][t_idx][2]
      if task_id == -1:
        if has_idle_label == False:
          plt.plot([start, end], [y, y], c='k', label=' Idle', linewidth=20.)
          has_idle_label = True
        else:
          plt.plot([start, end], [y, y], c='k', linewidth=20.)
      else:
        plt.plot([start, end], [y, y], label=' Node-'+str(task_id), linewidth=20.)
  xtick = range(0, int(task_info[-1][-1][-1] * 1.5), int(task_info[-1][-1][-1] * 0.15))
  plt.xticks(xtick)
  ytick = range(0, len(task_info) + 1, 1)
  plt.yticks(ytick)
  plt.legend(loc='upper right')
  plt.xlabel('time')
  plt.ylabel('function unit')
  plt.title('schedule result for each task in function units')
  plt.show()

if __name__ == '__main__':
  task_info = get_input('input')
  max_time = get_max_time(task_info)
  task_info = rearange_task(task_info, max_time)
  print_processor_usage(task_info)
