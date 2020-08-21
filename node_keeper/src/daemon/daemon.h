/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_DAEMON_DAEMON_H_
#define NODE_KEEPER_SRC_DAEMON_DAEMON_H_

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "cdcf/logger.h"
#include "src/daemon/process_manager.h"

class Daemon {
  ProcessManager& process_manager_;
  std::thread thread_;
  std::string path_;
  std::vector<std::string> args_;
  std::shared_ptr<void> app_process_info_;
  bool guard = true;
  size_t restart_ = 0;
  std::chrono::milliseconds stable_time_;
  std::function<void()> app_down_handle_;
  std::function<void()> node_keeper_clean_up_;

  void ExitIfProcessNotStable(
      const std::chrono::system_clock::time_point& start,
      const std::chrono::system_clock::time_point& end);

 public:
  Daemon(
      ProcessManager& process_manager, std::string path,
      std::vector<std::string> args,
      std::function<void()> app_down_handle = nullptr,
      std::function<void()> node_keeper_clean_up = nullptr,
      std::chrono::milliseconds stable_time = std::chrono::milliseconds(3000));
  void Run();
  void StopGuard();
  virtual ~Daemon();
  void Start();
};

#endif  // NODE_KEEPER_SRC_DAEMON_DAEMON_H_
