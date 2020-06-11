/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon.h>

Daemon::Daemon(ProcessManager& process_manager, std::string path,
               std::vector<std::string> args,
               std::chrono::milliseconds stable_time)
    : process_manager_(process_manager),
      path_(std::move(path)),
      args_(std::move(args)),
      stable_time_(stable_time) {}

void Daemon::Run() {
  using std::chrono::system_clock;
  app_process_info_ = process_manager_.NewProcessInfo();
  system_clock::time_point start;
  while (guard) {
    process_manager_.CreateProcess(path_, args_, app_process_info_);
    if (restart_ == 0) {
      start = system_clock::now();
    }
    //        std::cout << "pid: " << *((pid_t *)app_process_info_.get()) <<
    //        std::endl;
    process_manager_.WaitProcessExit(app_process_info_);
    if (restart_ == 0) {
      auto end = system_clock::now();
      ExitIfProcessNotStable(start, end);
    }
    restart_++;
  }
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
    exit(1);
  }
}
