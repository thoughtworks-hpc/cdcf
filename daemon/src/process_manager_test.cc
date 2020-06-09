/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "daemon/process_manager.h"

#include <gtest/gtest.h>

TEST(PosixProcessManager, DISABLED_should_create_process_correctly) {
  PosixProcessManager process_manager;

  process_manager.CreateProcess("/bin/ls", {"-l"});
}
