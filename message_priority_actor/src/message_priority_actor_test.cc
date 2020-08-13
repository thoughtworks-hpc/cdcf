/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/message_priority_actor.h"

#include <gtest/gtest.h>
#include <logger.h>

class CalculatorWithPriority : public MessagePriorityActor {
 public:
  explicit CalculatorWithPriority(caf::actor_config& cfg)
      : MessagePriorityActor(cfg) {}
  caf::behavior make_behavior() override {
    return {[=](int a, int b) -> int {
      caf::aout(this) << "received add task. input a:" << a << " b:" << b
                      << std::endl;

      int result = a + b;

      auto start = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed_seconds =
          std::chrono::milliseconds(0);
      std::chrono::duration<double> time_limit = std::chrono::milliseconds(200);
      while (elapsed_seconds < time_limit) {
        auto end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
      }

      caf::aout(this) << "return: " << result << std::endl;
      return result;
    }};
  }
};

using start_atom = caf::atom_constant<caf::atom("start")>;

struct dispatcher_state {
  std::vector<int> result;
};

caf::behavior job_dispatcher(caf::stateful_actor<dispatcher_state>* self,
                             caf::actor target) {
  return {[=](start_atom) {
            int task_num = 10;
            for (int i = 0; i < task_num; i++) {
              self->send(target, 1, 1);
              self->send(target, 2, 2);
            }
          },
          [=](int result) {

          }};
}

class ActorPriorityTest : public ::testing::Test {
  void SetUp() override {
    calculator_ = system_.spawn<CalculatorWithPriority>();
    dispatcher_ = system_.spawn(job_dispatcher, calculator_);
  }

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor dispatcher_;
  caf::actor calculator_;
};

TEST_F(ActorPriorityTest, happy_path) { EXPECT_EQ(1, 1); }