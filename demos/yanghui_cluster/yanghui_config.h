/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#include <actor_system.h>

#include <string>
#include <vector>

struct NumberCompareData {
  std::vector<int> numbers;
  int index;
  friend bool operator==(const NumberCompareData& lhs,
                         const NumberCompareData& rhs);
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const NumberCompareData& x) {
  return f(caf::meta::type_name("NumberCompareData"), x.numbers, x.index);
}

struct YanghuiTriangleData {
  std::vector<std::vector<int>> data;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const YanghuiTriangleData& x) {
  return f(caf::meta::type_name("YanghuiTriangleData"), x.data);
}

using calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<int>,
                     caf::replies_to<NumberCompareData>::with<int>>;

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

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;
  bool balance_mode = false;

  config() {
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(node_keeper_port, "node_port", "set node keeper port")
        .add(balance_mode, "balance_mode, b",
             "set current cluster mode, only work when root node");
    add_message_type<NumberCompareData>("NumberCompareData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
