/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon.h>
#include <daemon/process_manager.h>

Daemon::Daemon(ProcessManager& process_manager, std::string path,
               std::vector<std::string> args)
    : process_manager_(process_manager),
      path_(std::move(path)),
      args_(std::move(args)) {}

void Daemon::Run() {
  app_process_info_ = process_manager_.NewProcessInfo();

  while (guard) {
    process_manager_.CreateProcess(path_, args_, app_process_info_);
    //    std::cout << "pid: " << *((pid_t *)app_process_info_.get()) <<
    //    std::endl;
    process_manager_.WaitProcessExit(app_process_info_);
  }
}

Daemon::~Daemon() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Daemon::StopGuard() { guard = false; }

void Daemon::Start() { thread_ = std::thread(&Daemon::Run, this); }
