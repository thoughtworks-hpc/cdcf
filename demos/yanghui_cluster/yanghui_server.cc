/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_server.h"

caf::behavior mirror(caf::event_based_actor* self) {
  // return the (initial) actor behavior
  return {// a handler for messages containing a single string
          // that replies with a string
          [=](const std::string& what) -> std::string {
            std::cout << "start mirror 1." << std::endl;
            auto sender = self->current_sender();
            std::cout << "start mirror 2." << std::endl;
            std::cout << "start mirror 3： "
                      << caf::to_string(sender->address()) << std::endl;
            std::cout << "start mirror 4." << std::endl;
            // prints "Hello World!" via aout (thread-safe cout wrapper)
            aout(self) << what << std::endl;
            std::cout << "start mirror 5." << std::endl;
            // reply "!dlroW olleH"
            return std::string(what.rbegin(), what.rend());
          }};
}

void ErrorHandler(const caf::error& err) {
  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
}

caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* actor_guard) {
  return {[=](const YanghuiData& data) {
            std::cout << "start standard job counting." << std::endl;
            auto yanghui_data = data.data;
            std::cout << "yanghui_standard_job_actor_fun 0" << std::endl;
            //            caf::strong_actor_ptr message_sender =
            //            self->current_sender();
            self->state.message_sender = self->current_sender();
            std::cout << "yanghui_standard_job_actor_fun 1" << std::endl;
            actor_guard->SendAndReceive(
                [&](int result) { self->send(self, result); }, ErrorHandler,
                yanghui_data);
            std::cout << "yanghui_standard_job_actor_fun 2" << std::endl;
          },
          [=](int result) {
            self->send(caf::actor_cast<caf::actor>(self->state.message_sender),
                       true, result);
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
            std::cout << "load balance job result: " << final_result
                      << std::endl;
            self->send(caf::actor_cast<caf::actor>(self->state.message_sender),
                       true, final_result);
            std::cout << "load balance job 0 " << std::endl;
          }};
}

caf::behavior yanghui_router_pool_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* pool_guard) {
  return {[=](const YanghuiData& data) {
    std::cout << "start router pool job counting." << std::endl;
    auto yanghui_data = data.data;
    //            caf::strong_actor_ptr message_sender =
    //            self->current_sender();
    auto sender = self->current_sender();
    std::cout << "yanghui_router_pool_job_actor_sender: "
              << caf::to_string(sender->address()) << std::endl;
    pool_guard->SendAndReceive(
        [&](int result) {
          std::cout << "yanghui_router_pool_job_actor_fun 1: " << result
                    << std::endl;
          caf::aout(self) << "yanghui_router_pool_job_actor_fun 1" << std::endl;
          self->send(caf::actor_cast<caf::actor>(sender), true, result);
          std::cout << "yanghui_router_pool_job_actor_fun 2" << std::endl;
          caf::aout(self) << "yanghui_router_pool_job_actor_fun 2" << std::endl;
        },
        ErrorHandler, yanghui_data);
  }};
}