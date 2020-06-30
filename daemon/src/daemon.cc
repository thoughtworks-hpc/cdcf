/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon.h>

#include <iostream>

Daemon::Daemon(ProcessManager& process_manager, cdcf::Logger& logger,
               std::string path, std::vector<std::string> args,
               std::function<void()> app_down_handle,
               std::chrono::milliseconds stable_time)
    : process_manager_(process_manager),
      logger_(logger),
      path_(std::move(path)),
      args_(std::move(args)),
      stable_time_(stable_time),
      app_down_handle_(std::move(app_down_handle)) {}

void Daemon::Run() {
  using std::chrono::system_clock;
  app_process_info_ = process_manager_.NewProcessInfo();
  system_clock::time_point start;
  logger_.Info("start guard actor system");
  process_manager_.InstallSignalHandlersForQuit(app_process_info_, &guard);
  while (guard) {
    process_manager_.CreateProcess(path_, args_, app_process_info_);
    if (restart_ == 0) {
      start = system_clock::now();
    }
    //        std::cout << "pid: " << *((pid_t *)app_process_info_.get()) <<
    //        std::endl;
    process_manager_.WaitProcessExit(app_process_info_);
    if (app_down_handle_) {
      app_down_handle_();
    }
    logger_.Warn("actor system exit");
    if (restart_ == 0) {
      auto end = system_clock::now();
      ExitIfProcessNotStable(start, end);
    }
    restart_++;
  }
  logger_.Info("stop guard, node keeper exit");
  process_manager_.Exit(0);
}

Daemon::~Daemon() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Daemon::StopGuard() { guard = false; }

void Daemon::Start() { thread_ = std::thread(&Daemon::Run, this); }

void Daemon::ExitIfProcessNotStable(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
  auto diff =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  if (diff < stable_time_) {
    logger_.Error("actor system not stable, node_keeper will exit.");
    std::cout << "actor system not stable, node_keeper will exit." << std::endl;
    process_manager_.Exit(1);
  }
}
