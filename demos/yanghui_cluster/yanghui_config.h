/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#include <actor_system.h>

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../message_priority_actor/include/message_priority_actor.h"

struct NumberCompareData {
  std::vector<int> numbers;
  int index;
  friend bool operator==(const NumberCompareData& lhs,
                         const NumberCompareData& rhs);
  friend std::ostream& operator<<(std::ostream& os,
                                  const NumberCompareData& data);
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const NumberCompareData& x) {
  return f(caf::meta::type_name("NumberCompareData"), x.numbers, x.index);
}

struct ResultWithPosition {
  int result;
  int position;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const ResultWithPosition& x) {
  return f(caf::meta::type_name("ResultWithPosition"), x.result, x.position);
}

using calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<int>,
                     caf::replies_to<NumberCompareData>::with<int>,
                     caf::replies_to<int, int, int>::with<ResultWithPosition> >;

calculator::behavior_type calculator_fun(calculator::pointer self);

calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro);

class typed_calculator : public calculator::base {
 public:
  explicit typed_calculator(caf::actor_config& cfg) : calculator::base(cfg) {}

  behavior_type make_behavior() override {
    return sleep_calculator_fun(this, deal_msg_count, 0);
  }

  std::atomic_int deal_msg_count = 0;
};

class typed_slow_calculator : public calculator::base {
 public:
  explicit typed_slow_calculator(caf::actor_config& cfg)
      : calculator::base(cfg) {}

  behavior_type make_behavior() override {
    return sleep_calculator_fun(this, deal_msg_count, 1800);
  }

  std::atomic_int deal_msg_count = 0;
};

class CalculatorWithPriority : public MessagePriorityActor {
 public:
  CalculatorWithPriority(caf::actor_config& cfg) : MessagePriorityActor(cfg){};
  caf::behavior make_behavior() override {
    return {
        [=](int a, int b) -> int {
          caf::aout(this) << "received add task. input a:" << a << " b:" << b
                          << std::endl;

          int result = a + b;
          caf::aout(this) << "return: " << result << std::endl;
          return result;
        },
        // currently, for remotely spawned actor, it seems caf does not
        // support return types other than c++ primitive types and std::string
        [=](int a, int b, int position) -> ResultWithPosition {
          this->mailbox();
          caf::aout(this) << this->current_mailbox_element()->is_high_priority()
                          << " received add task. input a:" << a << " b:" << b
                          << std::endl;
          ResultWithPosition result = {a + b, position};

          auto start = std::chrono::steady_clock::now();
          std::chrono::duration<double> elapsed_seconds =
              std::chrono::milliseconds(0);
          std::chrono::duration<double> time_limit =
              std::chrono::milliseconds(300);
          while (elapsed_seconds < time_limit) {
            auto end = std::chrono::steady_clock::now();
            elapsed_seconds = end - start;
          }

          std::cout << "return: " << result.result << std::endl;
          return result;
        },
        [=](NumberCompareData& data) -> int {
          if (data.numbers.empty()) {
            caf::aout(this) << "get empty compare" << std::endl;
            return 999;
          }

          int result = data.numbers[0];

          caf::aout(this) << "received compare task, input: ";

          for (int number : data.numbers) {
            caf::aout(this) << number << " ";
            if (number < result) {
              result = number;
            }
          }

          caf::aout(this) << std::endl;
          caf::aout(this) << "return: " << result << std::endl;

          return result;
        }};
  }
};

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;
  int worker_load = 0;

  config() {
    add_actor_type("calculator", calculator_fun);
    add_actor_type<CalculatorWithPriority>("CalculatorWithPriority");
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(worker_load, "load, l", "load balance worker sleep second")
        .add(node_keeper_port, "node_port", "set node keeper port");
    add_message_type<NumberCompareData>("NumberCompareData");
    add_message_type<ResultWithPosition>("ResultWithPosition");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
