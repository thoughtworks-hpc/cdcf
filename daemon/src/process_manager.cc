/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon/process_manager.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
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
    PrintErrno("fork");
    Exit(1);
  }
  if (pid == 0) {
    // child

    struct sigaction act;
    act.sa_handler = SIG_IGN;

    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);

    std::cout << "[exec] app = " << path << std::endl;
    execv(path.c_str(), &argv.front());
    PrintErrno("exec");
    _exit(1);
  }
  // parent and child will use same stdin and stdout.
  // if child and parent both need read stdin, behavior will like:
  // input: 1 2 3 4
  // parent read: 1, 3
  // child read: 2 4

  // parent
  auto process_info =
      reinterpret_cast<process_info_t*>(child_process_info.get());
  *process_info = pid;
  //    while (1);
}

void PosixProcessManager::PrintErrno(const std::string& sys_call) const {
  std::cout << "[" << sys_call << " failed], error: " << strerror(errno)
            << " , errno: " << errno << std::endl;
}

std::shared_ptr<void> PosixProcessManager::NewProcessInfo() {
  return std::make_shared<process_info_t>();
}

void PosixProcessManager::WaitProcessExit(std::shared_ptr<void> process_info) {
  auto pid = *(reinterpret_cast<process_info_t*>(process_info.get()));
  int status;
  while (true) {
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      break;
    }
  }
}

PosixProcessManager::PosixProcessManager(cdcf::Logger& logger)
    : ProcessManager(logger) {}

static std::shared_ptr<void> g_pid;
static bool* g_guard;
static void handler(int) {
  auto pid = *reinterpret_cast<pid_t*>(g_pid.get());
  if (pid) {
    *g_guard = false;
    std::cout << "[Signal] get quit signal. Exiting..." << std::endl;
    kill(*reinterpret_cast<pid_t*>(g_pid.get()), SIGTERM);
  }
}

void PosixProcessManager::InstallSignalHandlersForQuit(
    std::shared_ptr<void> child_process_info, bool* guard) {
  struct sigaction act;
  g_pid = child_process_info;
  g_guard = guard;
  act.sa_handler = handler;

  sigaction(SIGINT, &act, nullptr);
  sigaction(SIGTERM, &act, nullptr);
  sigaction(SIGQUIT, &act, nullptr);
}

ProcessManager::ProcessManager(cdcf::Logger& logger) : logger_(logger) {}

void ProcessManager::Exit(int exit_code) { exit(exit_code); }
