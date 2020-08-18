/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/daemon/process_manager.h"

#include <gtest/gtest.h>

TEST(PosixProcessManager, ShouldCreateProcessCorrectly) {
  PosixProcessManager process_manager;

  auto process_info = process_manager.NewProcessInfo();
  process_manager.CreateProcess("/bin/ls", {"-l"}, process_info);
}

TEST(PosixProcessManager, ShouldReturnWhenProcessExit) {
  PosixProcessManager process_manager;

  auto process_info = process_manager.NewProcessInfo();
  process_manager.CreateProcess("/bin/ls", {"-l"}, process_info);
  process_manager.WaitProcessExit(process_info);
}
