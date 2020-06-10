/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef DAEMON_INCLUDE_DAEMON_H_
#define DAEMON_INCLUDE_DAEMON_H_

#include <string>
#include <thread>
#include <vector>

#include "daemon/process_manager.h"

class Daemon {
  ProcessManager& process_manager_;
  std::thread thread_;
  std::string path_;
  std::vector<std::string> args_;
  std::shared_ptr<void> app_process_info_;
  bool guard = true;

 public:
  // Todo(Yujia.Li): use default args
  Daemon(ProcessManager& process_manager, std::string path,
         std::vector<std::string> args);
  void Run();
  void StopGuard();
  virtual ~Daemon();
  void Start();
};

#endif  // DAEMON_INCLUDE_DAEMON_H_
