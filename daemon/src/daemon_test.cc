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
  MOCK_METHOD(void, WaitProcessExit, (std::shared_ptr<void>), (override));
};

TEST(Daemon, should_create_process_again_when_process_exit) {
  MockProcessManager mock_process_manager;
  const char *path = "/bin/ls";
  EXPECT_CALL(mock_process_manager, NewProcessInfo());
  EXPECT_CALL(mock_process_manager,
              CreateProcess(testing::_, testing::_, testing::_))
      .Times(2);
  EXPECT_CALL(mock_process_manager, WaitProcessExit(testing::_));

  Daemon d(mock_process_manager, path, {"-l"});
}
