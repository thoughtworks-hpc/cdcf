/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_
#define DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_

#include <string>
#include <vector>

class ProcessManager {
 public:
  virtual std::shared_ptr<void> NewProcessInfo() = 0;
  virtual void CreateProcess(const std::string& path,
                             const std::vector<std::string>& args,
                             std::shared_ptr<void> child_process_info) = 0;
};

class PosixProcessManager : public ProcessManager {
  using process_info_t = pid_t;

 public:
  std::shared_ptr<void> NewProcessInfo() override;
  void CreateProcess(const std::string& path,
                     const std::vector<std::string>& args,
                     std::shared_ptr<void> child_process_info) override;
};

#endif  // DAEMON_INCLUDE_DAEMON_PROCESS_MANAGER_H_
