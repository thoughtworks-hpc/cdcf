/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_server.h"

#include <limits.h>

caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    cdcf::actor_system::ActorGuard* actor_guard) {
  return {[=](const YanghuiData& data) {
    std::cout << "start standard job counting." << std::endl;
    auto yanghui_data = data.data;
    //            self->state.message_sender = self->current_sender();
    auto sender = self->current_sender();
    actor_guard->SendAndReceive(
        [&](int result) {
          self->send(caf::actor_cast<caf::actor>(sender), true, result);
        },
        [&](const caf::error& err) {
          std::cout << "call actor get error:" << caf::to_string(err)
                    << std::endl;
          self->send(caf::actor_cast<caf::actor>(sender), false, INT_MAX);
        },
        yanghui_data);
  }};
}

caf::behavior yanghui_priority_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, WorkerPool* worker_pool,
    caf::actor dispatcher) {
  return {
      [=](const YanghuiData& data) {
        std::cout << "start yanghui calculation with priority." << std::endl;
        auto yanghui_data = data.data;
        while (true) {
          std::cout << "waiting for worker" << std::endl;
          if (!worker_pool->IsEmpty()) {
            break;
          }
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        self->state.message_sender = self->current_sender();
        self->send(dispatcher, yanghui_data);
      },
      [=](std::vector<std::pair<bool, int>> result) {
        auto result_pair_1 = result[0];
        auto result_pair_2 = result[1];

        if (result.size() != 2 || !result_pair_1.first ||
            (result_pair_1.second != result_pair_2.second)) {
          self->send(caf::actor_cast<caf::actor>(self->state.message_sender),
                     false, 0);
        } else {
          self->send(caf::actor_cast<caf::actor>(self->state.message_sender),
                     true, result_pair_1.second);
        }
      }};
}

caf::behavior yanghui_load_balance_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    caf::actor yanghui_load_balance_count_path,
    caf::actor yanghui_load_balance_get_min) {
  return {[=](const YanghuiData& data) {
            std::cout << "start load balance job counting." << std::endl;
            auto yanghui_data = data.data;
            self->state.message_sender = self->current_sender();
            self->send(yanghui_load_balance_count_path, yanghui_data);
          },
          [=](const std::vector<int>& count_path_result) {
            self->send(yanghui_load_balance_get_min, count_path_result);
          },
          [=](int final_result) {
            self->send(caf::actor_cast<caf::actor>(self->state.message_sender),
                       true, final_result);
          }};
}

caf::behavior yanghui_router_pool_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    cdcf::actor_system::ActorGuard* pool_guard) {
  return {[=](const YanghuiData& data) {
    std::cout << "start router pool job counting." << std::endl;
    auto yanghui_data = data.data;
    auto sender = self->current_sender();
    pool_guard->SendAndReceive(
        [&](int result) {
          self->send(caf::actor_cast<caf::actor>(sender), true, result);
        },
        [&](const caf::error& err) {
          std::cout << "call actor get error:" << caf::to_string(err)
                    << std::endl;
          self->send(caf::actor_cast<caf::actor>(sender), false, INT_MAX);
        },
        yanghui_data);
  }};
}
