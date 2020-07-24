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

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "../../actor_fault_tolerance/include/actor_union.h"
#include "../../actor_monitor/include/actor_monitor.h"
#include "../../message_priority_actor/include/message_priority_actor.h"
#include "./include/yanghui_server.h"
#include "./include/yanghui_with_priority.h"

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

//
//
// const uint16_t yanghui_job_port1 = 55011;
// const uint16_t yanghui_job_port2 = 55012;
// const uint16_t yanghui_job_port3 = 55013;
// const uint16_t yanghui_job_port4 = 55014;
//
// struct yanghui_job_state {
//  caf::strong_actor_ptr message_sender;
//};
//
// void ErrorHandler(const caf::error& err) {
//  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
//}
//
// caf::behavior yanghui_standard_job_actor_fun(
//    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* actor_guard) {
//  return {[&](const YanghuiData& data) {
//    std::cout << "start standard job counting." << std::endl;
//    auto yanghui_data = data.data;
//    std::cout << "yanghui_standard_job_actor_fun 0" << std::endl;
//    //            caf::strong_actor_ptr message_sender =
//    //            self->current_sender();
//    caf::actor sender;
//    std::cout << "yanghui_standard_job_actor_fun 0.1: "
//              << caf::to_string(self->current_sender()->address())
//              << std::endl;
//    try {
//      std::cout << "yanghui_standard_job_actor_fun 0.2" << std::endl;
//      sender = caf::actor_cast<caf::actor>(self->current_sender());
//      std::cout << "yanghui_standard_job_actor_fun 0.3" << std::endl;
//    } catch (std::exception& e) {
//      std::cout << "yanghui_standard_job_actor_fun 0.5: " << e.what()
//                << std::endl;
//    }
//
//    std::cout << "yanghui_standard_job_actor_fun 1" << std::endl;
//    actor_guard->SendAndReceive(
//        [&](int result) { self->send(self, result); }, ErrorHandler,
//        yanghui_data);
//    std::cout << "yanghui_standard_job_actor_fun 2" << std::endl;
//  },
//          [=](int result) {
//            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
//                      true, result);
//          }};
//}
//
// caf::behavior yanghui_priority_job_actor_fun(
//    caf::stateful_actor<yanghui_job_state>* self, WorkerPool* worker_pool,
//    caf::actor dispatcher) {
//  return {[&](const YanghuiData& data) {
//    std::cout << "start yanghui calculation with priority."
//              << std::endl;
//    auto yanghui_data = data.data;
//    while (true) {
//      std::cout << "waiting for worker" << std::endl;
//      if (!worker_pool->IsEmpty()) {
//        break;
//      }
//      std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
//    std::cout << "yanghui_priority_job_actor_fun 0" << std::endl;
//    self->state.message_sender = self->current_sender();
//    std::cout << "yanghui_priority_job_actor_fun 1" << std::endl;
//    anon_send(dispatcher, yanghui_data);
//    std::cout << "yanghui_priority_job_actor_fun 2" << std::endl;
//  },
//          [=](std::vector<std::pair<bool, int>> result) {
//            auto result_pair_1 = result[0];
//            auto result_pair_2 = result[1];
//
//            if (result.size() != 2 || !result_pair_1.first ||
//                (result_pair_1.first != result_pair_2.first)) {
//              anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
//                        false, 0);
//            }
//
//            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
//                      true, result_pair_1.second);
//          }};
//}
//
// caf::behavior yanghui_load_balance_job_actor_fun(
//    caf::stateful_actor<yanghui_job_state>* self,
//    caf::actor yanghui_load_balance_count_path,
//    caf::actor yanghui_load_balance_get_min) {
//  return {[&](const YanghuiData& data) {
//    std::cout << "start load balance job counting." << std::endl;
//    auto yanghui_data = data.data;
//    self->state.message_sender = self->current_sender();
//    caf::anon_send(yanghui_load_balance_count_path, yanghui_data);
//  },
//          [=](const std::vector<int>& count_path_result) {
//            caf::anon_send(yanghui_load_balance_get_min, count_path_result);
//          },
//          [=](int final_result) {
//            caf::anon_send(
//                caf::actor_cast<caf::actor>(self->state.message_sender), true,
//                final_result);
//          }};
//}
//
// caf::behavior yanghui_router_pool_job_actor_fun(
//    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* pool_guard) {
//  return {[&](const YanghuiData& data) {
//    std::cout << "start router pool job counting." << std::endl;
//    auto yanghui_data = data.data;
//    //            caf::strong_actor_ptr message_sender =
//    //            self->current_sender();
//    self->state.message_sender = self->current_sender();
//    pool_guard->SendAndReceive(
//        [&](int result) { self->send(self, result); }, ErrorHandler,
//        yanghui_data);
//  },
//          [=](int result) {
//            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
//                      true, result);
//          }};
//}

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  uint16_t yanghui_job_port = 0;
  bool root = false;
  int worker_load = 0;

  config() {
    add_actor_type("calculator", calculator_fun);
    add_actor_type<CalculatorWithPriority>("CalculatorWithPriority");
    add_actor_type("yanghui_standard_job_actor",
                   yanghui_standard_job_actor_fun);
    add_actor_type("yanghui_priority_job_actor",
                   yanghui_priority_job_actor_fun);
    add_actor_type("yanghui_load_balance_job_actor",
                   yanghui_load_balance_job_actor_fun);
    add_actor_type("yanghui_router_pool_job_actor",
                   yanghui_router_pool_job_actor_fun);
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(yanghui_job_port, "yanghui_job_port", "set yanghui job port")
        .add(root, "root, r", "set current node be root")
        .add(worker_load, "load, l", "load balance worker sleep second")
        .add(node_keeper_port, "node_port", "set node keeper port");
    add_message_type<NumberCompareData>("NumberCompareData");
    add_message_type<ResultWithPosition>("ResultWithPosition");
    add_message_type<YanghuiData>("YanghuiData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
