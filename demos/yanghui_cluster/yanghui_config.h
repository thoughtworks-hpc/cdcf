/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_TRIANGLE_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_TRIANGLE_YANGHUI_CONFIG_H_
#include <string>
#include <vector>

#include "../../config_manager/include/cdcf_config.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

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
                     caf::replies_to<NumberCompareData>::with<int>>;

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](int a, int b) -> int {
            caf::aout(self) << "received add task. input a:" << a << " b:" << b
                            << std::endl;

            int result = a + b;
            caf::aout(self) << "return: " << result << std::endl;
            return result;
          },
          [=](NumberCompareData& data) -> int {
            if (data.numbers.empty()) {
              caf::aout(self) << "get empty compare" << std::endl;
              return 999;
            }

            int result = data.numbers[0];

            caf::aout(self) << "received compare task, input: ";

            for (int number : data.numbers) {
              caf::aout(self) << number << " ";
              if (number < result) {
                result = number;
              }
            }

            caf::aout(self) << std::endl;
            caf::aout(self) << "return: " << result << std::endl;

            return result;
          }};
}

class config : public CDCFConfig {
 public:
  uint16_t port = 0;
  std::string host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;

  config() {
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node")
        .add(worker_port, "worker, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(node_keeper_port, "node, n", "set node keeper port");
    add_message_type<NumberCompareData>("NumberCompareData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_TRIANGLE_YANGHUI_CONFIG_H_
