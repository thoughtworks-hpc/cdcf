/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "cdcf/actor_monitor.h"

#include <cdcf/logger.h>
#include <gtest/gtest.h>

caf::behavior test_init_actor(caf::event_based_actor* self) { return {}; }

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

caf::behavior calculator_with_error(caf::event_based_actor* self) {
  self->set_default_handler(caf::reflect_and_quit);
  return {[=](add_atom, int a, int b) {
            self->quit();
            return 0;
          },
          [=](sub_atom, int a, int b) -> int {
            CDCF_LOGGER_INFO(
                "received sub_atom task from remote node. input a:{} b:{}", a,
                b);
            return a - b;
          }};
}

class ActorMonitorTest : public ::testing::Test {
  void SetUp() override {
    test_init_ = system_.spawn(test_init_actor);
    calculator_ = system_.spawn(calculator_with_error);
  }

  void TearDown() override {}

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor test_init_;
  caf::actor supervisor_;
  caf::actor calculator_;
  std::promise<std::string> promise_;
};

TEST_F(ActorMonitorTest, happy_path) {
  caf::scoped_actor scoped_sender(system_);
  std::string error_message_ = "";
  supervisor_ = system_.spawn<cdcf::ActorMonitor>(
      [&](const caf::down_msg& downMsg, const std::string& actor_description) {
      });
  cdcf::SetMonitor(supervisor_, calculator_, "worker actor for testing");

  scoped_sender->request(calculator_, caf::infinite, sub_atom::value, 3, 1)
      .receive([=](int reseult) { EXPECT_EQ(2, reseult); },
               [=](caf::error err) { ASSERT_FALSE(true); });

  EXPECT_EQ("", error_message_);
}

TEST_F(ActorMonitorTest, should_report_down_msg_when_down_event_happen) {
  std::string error_message_ = "";
  auto event_based_sender =
      caf::actor_cast<caf::event_based_actor*>(test_init_);

  supervisor_ = system_.spawn<cdcf::ActorMonitor>(
      [&](const caf::down_msg& downMsg, const std::string& actor_description) {
        promise_.set_value(caf::to_string(downMsg.reason));
      });
  cdcf::SetMonitor(supervisor_, calculator_, "worker actor for testing");

  event_based_sender->send(calculator_, add_atom::value, 1, 2);
  error_message_ = promise_.get_future().get();
  EXPECT_EQ("none", error_message_);
}

TEST_F(ActorMonitorTest, should_report_exit_msg_when_exit_event_happen) {
  std::string error_message_ = "";
  auto event_based_sender =
      caf::actor_cast<caf::event_based_actor*>(test_init_);

  supervisor_ = system_.spawn<cdcf::ActorMonitor>(
      [&](const caf::down_msg& downMsg, const std::string& actor_description) {
        promise_.set_value(caf::to_string(downMsg.reason));
      });
  cdcf::SetMonitor(supervisor_, calculator_, "worker actor for testing");

  event_based_sender->send_exit(calculator_, caf::exit_reason::kill);

  error_message_ = promise_.get_future().get();
  EXPECT_EQ("exit_reason(kill)", error_message_);
}

TEST_F(ActorMonitorTest, should_report_error_msg_when_error_event_message) {
  std::string error_message_ = "";
  auto event_based_sender =
      caf::actor_cast<caf::event_based_actor*>(test_init_);

  supervisor_ = system_.spawn<cdcf::ActorMonitor>(
      [&](const caf::down_msg& downMsg, const std::string& actor_description) {
        promise_.set_value(caf::to_string(downMsg.reason));
      });
  cdcf::SetMonitor(supervisor_, calculator_, "worker actor for testing");

  event_based_sender->send(calculator_,
                           caf::make_error(caf::exit_reason::out_of_workers));
  error_message_ = promise_.get_future().get();
  EXPECT_EQ("exit_reason(out_of_workers)", error_message_);
}

TEST_F(ActorMonitorTest, should_report_default_msg_when_send_unknown_message) {
  std::string error_message_ = "";
  auto event_based_sender =
      caf::actor_cast<caf::event_based_actor*>(test_init_);

  supervisor_ = system_.spawn<cdcf::ActorMonitor>(
      [&](const caf::down_msg& downMsg, const std::string& actor_description) {
        promise_.set_value(caf::to_string(downMsg.reason));
      });
  cdcf::SetMonitor(supervisor_, calculator_, "worker actor for testing");

  event_based_sender->send(calculator_, "unknown message");

  error_message_ = promise_.get_future().get();
  EXPECT_EQ("none", error_message_);
}
