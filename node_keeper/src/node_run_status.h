/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_NODE_RUN_STATUS_H_
#define NODE_KEEPER_SRC_NODE_RUN_STATUS_H_
#include <cstdio>
#include <memory>
#include <mutex>

struct MemoryStatus {
  uint64_t max_memory;
  uint64_t use_memory;
  double useRate;
};

class NodeRunStatus {
 public:
  static NodeRunStatus *GetInstance();
  virtual double GetCpuRate();
  virtual int GetMemoryState(MemoryStatus &memory_info);

 protected:
  NodeRunStatus(FILE *memoryFile, FILE *cpuFile);

 private:
  struct CpuInfo {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t io_wait;
    uint64_t irq;
    uint64_t soft_irq;
  };


  void GetCpuInfo(CpuInfo &cpu_info);

  FILE *memory_file;
  FILE *cpu_file;
};

#endif  // NODE_KEEPER_SRC_NODE_RUN_STATUS_H_
