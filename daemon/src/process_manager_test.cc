/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "daemon/process_manager.h"

#include <gtest/gtest.h>

TEST(PosixProcessManager, should_create_process_correctly) {
  PosixProcessManager process_manager;

  auto process_info = process_manager.NewProcessInfo();
  process_manager.CreateProcess("/bin/ls", {"-l"}, process_info);
}

TEST(PosixProcessManager, should_return_when_process_exit) {
  PosixProcessManager process_manager;

  auto process_info = process_manager.NewProcessInfo();
  process_manager.CreateProcess("/bin/ls", {"-l"}, process_info);
  process_manager.WaitProcessExit(process_info);
}
