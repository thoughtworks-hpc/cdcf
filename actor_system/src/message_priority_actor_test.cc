/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "cdcf/message_priority_actor.h"

#include <cdcf/logger.h>
#include <gtest/gtest.h>

class CalculatorWithPriority : public cdcf::MessagePriorityActor {
 public:
  explicit CalculatorWithPriority(caf::actor_config& cfg)
      : cdcf::MessagePriorityActor(cfg) {}
  caf::behavior make_behavior() override {
    return {[=](int a, int b) -> int {
      CDCF_LOGGER_INFO("received add task. input a:{} b:{}", a, b);

      int result = a + b;

      auto start = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed_seconds =
          std::chrono::milliseconds(0);
      std::chrono::duration<double> time_limit = std::chrono::milliseconds(10);
      while (elapsed_seconds < time_limit) {
        auto end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
      }

      CDCF_LOGGER_INFO("return: {}", result);
      return result;
    }};
  }
};

using start_atom = caf::atom_constant<caf::atom("start")>;

struct dispatcher_state {
  int first_job_times = 0;
  int second_job_times = 0;
  caf::strong_actor_ptr sender;
};

caf::behavior job_dispatcher(caf::stateful_actor<dispatcher_state>* self,
                             caf::actor target, bool is_first_job_with_priority,
                             bool is_second_job_with_priority) {
  const int task_num = 5;
  const int first_job_data = 1;
  const int second_job_data = 2;
  return {[=](start_atom) {
            self->state.sender = self->current_sender();

            for (int i = 0; i < task_num; i++) {
              if (!is_first_job_with_priority) {
                self->send(target, first_job_data, first_job_data);
              } else {
                self->send(target, cdcf::high_priority_atom::value,
                           first_job_data, first_job_data);
              }
            }

            for (int i = 0; i < task_num; i++) {
              if (!is_second_job_with_priority) {
                self->send(target, second_job_data, second_job_data);
              } else {
                self->send(target, cdcf::high_priority_atom::value,
                           second_job_data, second_job_data);
              }
            }
          },
          [=](int result) {
            if (result == first_job_data + first_job_data) {
              self->state.first_job_times += 1;
            } else if (result == second_job_data + second_job_data) {
              self->state.second_job_times += 1;
            }

            if (self->state.second_job_times == task_num) {
              self->send(caf::actor_cast<caf::actor>(self->state.sender), true);
            } else if (self->state.first_job_times == task_num) {
              self->send(caf::actor_cast<caf::actor>(self->state.sender),
                         false);
            }
          }};
}

class ActorPriorityTest : public ::testing::Test {
  void SetUp() override {
    calculator_ = system_.spawn<CalculatorWithPriority>();
    dispatcher_with_priority =
        system_.spawn(job_dispatcher, calculator_, false, true);
    dispatcher_all_normal_priority_ =
        system_.spawn(job_dispatcher, calculator_, false, false);
    dispatcher_all_high_priority_ =
        system_.spawn(job_dispatcher, calculator_, true, true);
  }

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor calculator_;
  caf::actor dispatcher_with_priority;
  caf::actor dispatcher_all_normal_priority_;
  caf::actor dispatcher_all_high_priority_;
};

TEST_F(ActorPriorityTest, should_get_result_as_priority_order) {
  caf::scoped_actor scoped_sender(system_);
  scoped_sender->request(dispatcher_with_priority, caf::infinite,
                         start_atom::value);
  scoped_sender->receive([=](bool result) { EXPECT_EQ(true, result); },
                         [=](caf::error err) { ASSERT_FALSE(true); });
}

TEST_F(ActorPriorityTest, should_get_result_as_send_order) {
  caf::scoped_actor scoped_sender(system_);
  scoped_sender->request(dispatcher_all_normal_priority_, caf::infinite,
                         start_atom::value);
  scoped_sender->receive([=](bool result) { EXPECT_EQ(false, result); },
                         [=](caf::error err) { ASSERT_FALSE(true); });
}

TEST_F(ActorPriorityTest,
       should_get_result_as_send_order_with_all_high_priority) {
  caf::scoped_actor scoped_sender(system_);
  scoped_sender->request(dispatcher_all_high_priority_, caf::infinite,
                         start_atom::value);
  scoped_sender->receive([=](bool result) { EXPECT_EQ(false, result); },
                         [=](caf::error err) { ASSERT_FALSE(true); });
}
