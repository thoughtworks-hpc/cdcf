/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "daemon.h"

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
};

TEST(Daemon, should_guard_process_until_stop_guard) {
  cdcf::StdoutLogger logger;
  MockProcessManager mock_process_manager(logger);
  const char *path = "/bin/ls";

  Daemon d(mock_process_manager, logger, path, {"-l"},
           std::chrono::milliseconds(50));
  EXPECT_CALL(mock_process_manager, NewProcessInfo());
  EXPECT_CALL(mock_process_manager,
              CreateProcess(testing::_, testing::_, testing::_))
      .Times(2);
  EXPECT_CALL(mock_process_manager, WaitProcessExit(testing::_))
      .WillOnce(testing::InvokeWithoutArgs([]() {
        using std::literals::operator""ms;
        std::this_thread::sleep_for(100ms);
      }))
      .WillOnce(testing::InvokeWithoutArgs([p = &d]() { p->StopGuard(); }));

  d.Start();
}

TEST(Daemon, should_exit_when_process_not_stable) {
  class FakeProcessManager : public ProcessManager {
   public:
    explicit FakeProcessManager(cdcf::Logger &logger)
        : ProcessManager(logger) {}
    std::shared_ptr<void> NewProcessInfo() override {
      return std::shared_ptr<void>();
    }
    void CreateProcess(const std::string &path,
                       const std::vector<std::string> &args,
                       std::shared_ptr<void> child_process_info) override {}
    void WaitProcessExit(std::shared_ptr<void> process_info) override {
      using std::literals::operator""ms;
      std::this_thread::sleep_for(50ms);
    }
  };
  cdcf::StdoutLogger logger;
  FakeProcessManager process_manager(logger);
  const char *path = "/bin/ls";
  Daemon d(process_manager, logger, path, {"-l"});

  EXPECT_EXIT(d.Run(), testing::ExitedWithCode(1), "");
}
