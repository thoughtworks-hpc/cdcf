/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "daemon.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockProcessManager : public ProcessManager {
 public:
  MOCK_METHOD(std::shared_ptr<void>, NewProcessInfo, (), (override));
  MOCK_METHOD(void, CreateProcess,
              (const std::string &, const std::vector<std::string> &,
               std::shared_ptr<void>),
              (override));
};

TEST(ADaemon, should_call_process_manager_to_create_pocess) {
  MockProcessManager mock_process_manager;
  const char *path = "/bin/ls";
  std::vector<std::string> args{"-l"};
  EXPECT_CALL(mock_process_manager, NewProcessInfo());
  EXPECT_CALL(mock_process_manager,
              CreateProcess(path, args, testing::_));

  Daemon d(mock_process_manager, path, args);
}
