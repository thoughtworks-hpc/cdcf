/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#include <cdcf/actor_system.h>

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "../../actor_fault_tolerance/include/actor_union.h"
#include "../../actor_monitor/include/actor_monitor.h"
#include "../../message_priority_actor/include/message_priority_actor.h"
#include "./include/yanghui_with_priority.h"

struct YanghuiData {
  YanghuiData() {}
  explicit YanghuiData(std::vector<std::vector<int>> data) {
    this->data = data;
  }
  std::vector<std::vector<int>> data;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, const YanghuiData& x) {
  return f(caf::meta::type_name("YanghuiData"), x.data);
}

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
                     caf::replies_to<int, int, int>::with<ResultWithPosition>>;

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
  explicit CalculatorWithPriority(caf::actor_config& cfg)
      : MessagePriorityActor(cfg) {}
  caf::behavior make_behavior() override;
};

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  std::string node_keeper_host = "127.0.0.1";
  uint16_t node_keeper_port = 50051;
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
        .add(node_keeper_host, "node_keeper_host",
             "node keeper host, if not localhost need set")
        .add(node_keeper_port, "node_port",
             "set node keeper port, default is 4445");
    add_message_type<NumberCompareData>("NumberCompareData");
    add_message_type<ResultWithPosition>("ResultWithPosition");
    add_message_type<YanghuiData>("YanghuiData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
