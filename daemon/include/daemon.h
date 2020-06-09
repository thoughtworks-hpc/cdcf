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

 public:
  Daemon(ProcessManager& process_manager, const std::string& path,
         std::vector<std::string> args);
  void Run();
  virtual ~Daemon();
};

#endif  // DAEMON_INCLUDE_DAEMON_H_
