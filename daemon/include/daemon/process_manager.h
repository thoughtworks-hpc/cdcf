/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_
#define DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_

#include <string>
#include <vector>

class ProcessManager {
 public:
  virtual void CreateProcess(const std::string& path,
                             const std::vector<std::string>& args) = 0;
};

class PosixProcessManager : public ProcessManager {
 public:
  void CreateProcess(const std::string& path,
                     const std::vector<std::string>& args) override;
};

#endif  // DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_
