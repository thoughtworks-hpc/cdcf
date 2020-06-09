/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon.h>
#include <daemon/process_manager.h>

Daemon::Daemon(ProcessManager& process_manager, const std::string& path,
               std::vector<std::string> args)
    : process_manager_(process_manager), path_(path), args_(args) {
  thread_ = std::thread(&Daemon::Run, this);
}
void Daemon::Run() { process_manager_.CreateProcess(path_, args_); }
Daemon::~Daemon() {
  if (thread_.joinable()) {
    thread_.join();
  }
}
