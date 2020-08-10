/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/node_run_status.h"

#include <logger.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

static const char *kCpuInfoFilePath = "/proc/stat";
static const char *kMemoryInfoFilePath = "/proc/meminfo";

NodeRunStatus *NodeRunStatus::GetInstance() {
  static NodeRunStatus *instance;
  //    static std::once_flag flag;
  //    std::call_once(flag, [=]() {
  //        std::cout << "call once ..............." << std::endl;
  //    });

  if (nullptr == instance) {
    FILE *cpu_info = fopen(kCpuInfoFilePath, "r");
    FILE *memory_info = fopen(kMemoryInfoFilePath, "r");

    if (nullptr != cpu_info && nullptr != memory_info) {
      instance = new NodeRunStatus(memory_info, cpu_info);
    } else {
      instance = nullptr;
      if (nullptr == cpu_info) {
        std::cout << "init NodeRunStatus singleton failed. cpu info read form "
                  << std::string(kCpuInfoFilePath) << " failed." << std::endl;
      } else {
        std::cout
            << "init NodeRunStatus singleton failed. memory info read form "
            << std::string(kMemoryInfoFilePath) << " failed." << std::endl;
      }
    }
  }

  return instance;
}

NodeRunStatus::NodeRunStatus(FILE *memoryFile, FILE *cpuFile)
    : memory_file(memoryFile), cpu_file(cpuFile) {}

void NodeRunStatus::GetCpuInfo(CpuInfo &cpu_info) {
  char buff[256];
  char cpu_name[20];

  // first time read
  fflush(cpu_file);
  rewind(cpu_file);

  fgets(buff, sizeof(buff), cpu_file);

  sscanf(buff, "%s %llu %llu %llu %llu %llu %llu %llu", cpu_name,
         &cpu_info.user, &cpu_info.nice, &cpu_info.system, &cpu_info.idle,
         &cpu_info.io_wait, &cpu_info.irq, &cpu_info.soft_irq);
}

double NodeRunStatus::GetCpuRate() {
  CpuInfo cpu_info1{};
  CpuInfo cpu_info2{};

  GetCpuInfo(cpu_info1);
  usleep(100000);
  GetCpuInfo(cpu_info2);

  uint64_t sum = (cpu_info2.user + cpu_info2.nice + cpu_info2.idle +
                  cpu_info2.io_wait + cpu_info2.irq + cpu_info2.soft_irq) -
                 (cpu_info1.user + cpu_info1.nice + cpu_info1.idle +
                  cpu_info1.io_wait + cpu_info1.irq + cpu_info1.soft_irq);
  uint64_t idle = cpu_info2.idle - cpu_info1.idle;

  return static_cast<double>(sum - idle) / static_cast<double>(sum);
}

static const char *kMemoryTotal = "MemTotal:";
static const char *kMemoryAvailable = "MemAvailable:";

int NodeRunStatus::GetMemoryState(MemoryStatus &memory_info) {
  char buff[256];
  char check_str[20];
  uint64_t memory_total;
  // unsigned long memory_free;
  uint64_t memory_available;

  fflush(memory_file);
  rewind(memory_file);
  fgets(buff, sizeof(buff), memory_file);

  sscanf(buff, "%s %llu ", check_str, &memory_total);
  if (0 != strcmp(check_str, kMemoryTotal)) {
    CDCF_LOGGER_ERROR("line head({}) not match when read memory file",
                      kMemoryTotal);
    //    std::cout << "line head(" << kMemoryTotal
    //              << ") not match when read memory file" << std::endl;
    return -1;
  }

  // free memory skip
  fgets(buff, sizeof(buff), memory_file);
  // strtoul(buff, &check_str, 10);

  fgets(buff, sizeof(buff), memory_file);
  // memory_available = strtoul(buff, &check_str, 10);
  sscanf(buff, "%s %llu ", check_str, &memory_available);
  if (0 != strcmp(check_str, kMemoryAvailable)) {
    //    std::cout << "line head(" << kMemoryAvailable
    //              << ") not match when read memory file" << std::endl;
    CDCF_LOGGER_ERROR("line head({}) not match when read memory file",
                      kMemoryAvailable);
    return -1;
  }

  memory_info.max_memory = memory_total;
  memory_info.use_memory = memory_total - memory_available;
  memory_info.useRate = static_cast<double>(memory_info.use_memory) * 1.0 /
                        static_cast<double>(memory_total);

  return 0;
}
