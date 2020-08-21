/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_DAEMON_PROCESS_MANAGER_H_
#define NODE_KEEPER_SRC_DAEMON_PROCESS_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "cdcf/logger.h"

class ProcessManager {
 public:
  virtual std::shared_ptr<void> NewProcessInfo() = 0;
  virtual void InstallSignalHandlersForQuit(
      std::shared_ptr<void> child_process_info, bool* guard) = 0;
  virtual void CreateProcess(const std::string& path,
                             const std::vector<std::string>& args,
                             std::shared_ptr<void> child_process_info) = 0;
  virtual void WaitProcessExit(std::shared_ptr<void> process_info) = 0;
  virtual void Exit(int exit_code);
};

class PosixProcessManager : public ProcessManager {
  using process_info_t = pid_t;
  void PrintErrno(const std::string& sys_call) const;

 public:
  std::shared_ptr<void> NewProcessInfo() override;
  void InstallSignalHandlersForQuit(std::shared_ptr<void> child_process_info,
                                    bool* guard) override;
  void CreateProcess(const std::string& path,
                     const std::vector<std::string>& args,
                     std::shared_ptr<void> child_process_info) override;
  void WaitProcessExit(std::shared_ptr<void> process_info) override;
};

#endif  // NODE_KEEPER_SRC_DAEMON_PROCESS_MANAGER_H_
