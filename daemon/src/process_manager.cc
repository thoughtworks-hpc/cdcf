/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon/process_manager.h>
#include <unistd.h>

#include <iostream>

void PosixProcessManager::CreateProcess(
    const std::string& path, const std::vector<std::string>& args,
    std::shared_ptr<void> child_process_info) {
  std::vector<char*> argv{const_cast<char*>(path.c_str())};

  for (auto& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  auto pid = fork();
  if (pid < 0) {
    // TODO(Yujia.Li): log
    std::cout << "fork failed" << std::endl;
    return;
  }
  if (pid == 0) {
    // child
    execv(path.c_str(), &argv.front());
    std::cout << "execv failed" << std::endl;
    return;
  }
  // parent and child will use same stdin and stdout.
  // if child and parent both need read stdin, behavior will like:
  // input: 1 2 3 4
  // parent read: 1, 3
  // child read: 2 4

  // parent
  auto process_info = (process_info_t*)child_process_info.get();
  *process_info = pid;
//    while (1);
}
std::shared_ptr<void> PosixProcessManager::NewProcessInfo() {
  return std::make_shared<process_info_t>();
}
