/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef UNTITLED_NODERUNSTATUS_H
#define UNTITLED_NODERUNSTATUS_H
#include <cstdio>
#include <memory>
#include <mutex>

struct MemoryStatus {
  unsigned long max_memory;
  unsigned long use_memory;
  double useRate;
};

class NodeRunStatus {
 public:
  static NodeRunStatus *GetInstance();
  double GetCpuRate();
  int GetMemoryState(MemoryStatus &memory_info);

 private:
  struct CpuInfo {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long io_wait;
    unsigned long irq;
    unsigned long soft_irq;
  };

  NodeRunStatus(FILE *memoryFile, FILE *cpuFile);
  void GetCpuInfo(CpuInfo &cpu_info);

  FILE *memory_file;
  FILE *cpu_file;
};

#endif  // UNTITLED_NODERUNSTATUS_H
