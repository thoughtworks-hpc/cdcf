/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#include <actor_system.h>

#include <sstream>
#include <string>
#include <vector>

using all_atom = caf::atom_constant<caf::atom("all")>;

struct AllActorData {
  std::vector<caf::actor> actors;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, const AllActorData& x) {
  return f(caf::meta::type_name("AllActorData"), x.actors);
}

struct NumberCompareData {
  std::vector<int> numbers;
  int index;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const NumberCompareData& x) {
  return f(caf::meta::type_name("NumberCompareData"), x.numbers, x.index);
}

using calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<int>,
                     caf::replies_to<NumberCompareData>::with<int> >;

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](int a, int b) -> int {
            CDCF_LOGGER_DEBUG("received add task. input a:{} b: {}", a, b);

            int result = a + b;
            CDCF_LOGGER_DEBUG("return: {}", result);
            return result;
          },
          [=](NumberCompareData& data) -> int {
            if (data.numbers.empty()) {
              CDCF_LOGGER_DEBUG("get empty compare");
              return 999;
            }

            int result = data.numbers[0];
            std::stringstream input_ss;

            for (int number : data.numbers) {
              input_ss << number << " ";
              if (number < result) {
                result = number;
              }
            }
            CDCF_LOGGER_DEBUG("received compare task, input: {}",
                              input_ss.str());
            CDCF_LOGGER_DEBUG("return: {}", result);

            return result;
          }};
}

class typed_calculator : public calculator::base {
 public:
  explicit typed_calculator(caf::actor_config& cfg) : calculator::base(cfg) {}

  behavior_type make_behavior() override { return calculator_fun(this); }
};

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;

  config() {
    add_message_type<AllActorData>("AllActorData");
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(node_keeper_port, "node_port", "set node keeper port");
    add_message_type<NumberCompareData>("NumberCompareData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
