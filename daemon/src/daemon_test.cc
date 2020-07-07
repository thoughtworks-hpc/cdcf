/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <daemon.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockProcessManager : public ProcessManager {
 public:
  explicit MockProcessManager(cdcf::Logger &logger) : ProcessManager(logger) {}
  MOCK_METHOD(std::shared_ptr<void>, NewProcessInfo, (), (override));
  MOCK_METHOD(void, CreateProcess,
              (const std::string &, const std::vector<std::string> &,
               std::shared_ptr<void>),
              (override));
  MOCK_METHOD(void, WaitProcessExit, (std::shared_ptr<void>), (override));
  MOCK_METHOD(void, InstallSignalHandlersForQuit,
              (std::shared_ptr<void>, bool *), (override));
  MOCK_METHOD(void, Exit, (int), (override));
};

TEST(Daemon, should_guard_process_until_stop_guard) {
  cdcf::StdoutLogger logger("console");
  MockProcessManager mock_process_manager(logger);
  const char *path = "/bin/ls";

  Daemon d(mock_process_manager, logger, path, {"-l"}, nullptr, nullptr,
           std::chrono::milliseconds(10));
  EXPECT_CALL(mock_process_manager, NewProcessInfo());
  EXPECT_CALL(mock_process_manager,
              InstallSignalHandlersForQuit(testing::_, testing::_));
  EXPECT_CALL(mock_process_manager,
              CreateProcess(testing::_, testing::_, testing::_))
      .Times(2);
  EXPECT_CALL(mock_process_manager, WaitProcessExit(testing::_))
      .WillOnce(testing::InvokeWithoutArgs([]() {
        using std::literals::operator""ms;
        std::this_thread::sleep_for(40ms);
      }))
      .WillOnce(testing::InvokeWithoutArgs([p = &d]() { p->StopGuard(); }));
  EXPECT_CALL(mock_process_manager, Exit(0));

  d.Start();
}

TEST(Daemon, should_exit_when_process_not_stable) {
  cdcf::StdoutLogger logger("console");
  MockProcessManager mock_process_manager(logger);
  const char *path = "/bin/ls";
  Daemon d(mock_process_manager, logger, path, {"-l"});
  EXPECT_CALL(mock_process_manager, NewProcessInfo());
  EXPECT_CALL(mock_process_manager,
              InstallSignalHandlersForQuit(testing::_, testing::_));
  EXPECT_CALL(mock_process_manager,
              CreateProcess(testing::_, testing::_, testing::_));
  EXPECT_CALL(mock_process_manager, WaitProcessExit(testing::_))
      .WillOnce(testing::InvokeWithoutArgs([]() {
        using std::literals::operator""ms;
        std::this_thread::sleep_for(10ms);
      }));
  EXPECT_CALL(mock_process_manager, Exit(1))
      .WillOnce(testing::InvokeWithoutArgs([p = &d]() { p->StopGuard(); }));
  EXPECT_CALL(mock_process_manager, Exit(0));

  d.Start();
}
