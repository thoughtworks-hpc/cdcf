/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_server.h"

yanghui_job_priority_actor::behavior_type yanghui_job_priority_actor_fun(
    WorkerPool& worker_pool, caf::actor& dispatcher) {
  return {[=](const std::vector<std::vector<int>>& yanghui_data) {
    std::cout << "start yanghui calculation with priority." << std::endl;
    while (true) {
      std::cout << "waiting for worker" << std::endl;
      if (!worker_pool.IsEmpty()) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    caf::scoped_actor self{system};

    self->send(dispatcher, yanghui_data);
    self->receive(
        [=](std::vector<std::pair<bool, int>> result) {
          for (const auto& pair : result) {
            if (pair.first) {
              std::cout << "high priority with final result: " << pair.second
                        << std::endl;
            } else {
              std::cout << "normal priority with final result: " << pair.second
                        << std::endl;
            }
          }
        },
        [&](caf::error error) {
          std::cout << "Receive yanghui priority result error: "
                    << system.render(error) << std::endl;
        });
  }};