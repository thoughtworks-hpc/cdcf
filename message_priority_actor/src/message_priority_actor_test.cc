/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/message_priority_actor.h"

#include <gtest/gtest.h>
//#include <logger.h>

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
      std::chrono::duration<double> time_limit = std::chrono::milliseconds(100);
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
  int normal_priority_times = 0;
  int high_priority_times = 0;
  caf::strong_actor_ptr sender;
};

caf::behavior job_dispatcher(caf::stateful_actor<dispatcher_state>* self,
                             caf::actor target, bool with_priority) {
  int task_num = 20;
  return {[=](start_atom) {
            self->state.sender = self->current_sender();
            for (int i = 0; i < task_num; i++) {
              self->send(target, 1, 1);
            }
            for (int i = 0; i < task_num; i++) {
              if (with_priority) {
                self->send(target, high_priority_atom::value, 2, 2);
              } else {
                self->send(target, 2, 2);
              }
            }
          },
          [=](int result) {
            if (result == 2) {
              self->state.normal_priority_times += 1;
            } else {
              self->state.high_priority_times += 1;
            }

            if (self->state.high_priority_times == task_num) {
              self->send(caf::actor_cast<caf::actor>(self->state.sender), true);
            } else if (self->state.normal_priority_times == task_num) {
              self->send(caf::actor_cast<caf::actor>(self->state.sender),
                         false);
            }
          }};
}

class ActorPriorityTest : public ::testing::Test {
  void SetUp() override {
    calculator_ = system_.spawn<CalculatorWithPriority>();
    dispatcher_with_priority = system_.spawn(job_dispatcher, calculator_, true);
    dispatcher_without_priority_ =
        system_.spawn(job_dispatcher, calculator_, false);
  }

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor calculator_;
  caf::actor dispatcher_with_priority;
  caf::actor dispatcher_without_priority_;
};

TEST_F(ActorPriorityTest, happy_path) {
  caf::scoped_actor scoped_sender(system_);
  scoped_sender->request(dispatcher_with_priority, caf::infinite,
                         start_atom::value);
  scoped_sender->receive([=](bool result) { EXPECT_EQ(true, result); },
                         [=](caf::error err) { ASSERT_FALSE(true); });
}

TEST_F(ActorPriorityTest, should_get_result_as_send_order) {
  caf::scoped_actor scoped_sender(system_);
  scoped_sender->request(dispatcher_without_priority_, caf::infinite,
                         start_atom::value);
  scoped_sender->receive([=](bool result) { EXPECT_EQ(false, result); },
                         [=](caf::error err) { ASSERT_FALSE(true); });
}