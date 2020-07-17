/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <utility>

#include "./yanghui_config.h"
#include "./yanghui_simple_actor.h"

caf::behavior yanghui_get_final_result(caf::stateful_actor<GetMinState>* self,
                                       const caf::actor& worker,
                                       const caf::actor& out_data) {
  const int batch = 3;
  return {[=](const std::vector<int>& data) {
            int len = data.size();
            if (1 == len) {
              // std::cout << "final result:" << data[0] << std::endl;
              self->send(out_data, data[0]);
              return;
            }

            int count = 0;
            int more = 0;

            if (0 != len % batch) {
              more = 1;
            }

            self->state.count = len / batch + more;
            self->state.current_result.clear();

            for (int i = 0; i < len; i += batch) {
          int endIndex = i + batch < len ? i + batch : len;
          NumberCompareData send_data;
          send_data.numbers =
              std::vector<int>(data.begin() + i, data.begin() + endIndex);
          send_data.index = count;

          auto sender =
              caf::actor_cast<caf::strong_actor_ptr>(self->current_sender());
          auto self_actor = caf::actor_cast<caf::actor>(std::move(sender));
          // self->send(worker, send_data, self_actor);

          self->request(worker, caf::infinite, send_data, self_actor)
              .then([=](int result, int id) {
                std::cout << " compare complete:" << send_data.index
                          << std::endl;
                if (0 == self->state.current_result.count(id)) {
                  self->state.current_result.emplace(id, result);
                }
                if (self->state.count == self->state.current_result.size()) {
                  std::vector<int> newData;
                  for (auto& mapMember : self->state.current_result) {
                    newData.emplace_back(mapMember.second);
                  }

                  self->send(self, newData);
                }
              });
          count++;
        }
      },
          [=](int result, int id) {
            if (0 == self->state.current_result.count(id)) {
              self->state.current_result.emplace(id, result);
            }
            if (self->state.count == self->state.current_result.size()) {
              std::vector<int> newData;
              for (auto& mapMember : self->state.current_result) {
                newData.emplace_back(mapMember.second);
              }

              self->send(self, newData);
            }
          }};
}

caf::behavior yanghui_count_path(caf::stateful_actor<YanghuiState>* self,
                                 const caf::actor& worker,
                                 const caf::actor& out_data) {
  return {
      [=](const std::vector<std::vector<int> >& data) {
        self->state.data = data;
        self->state.current_result[0] = data[0][0];
        self->send(self, 1);
      },
      [=](int index) {
        for (auto mapValue : self->state.current_result) {
          self->state.data[index - 1][mapValue.first] = mapValue.second;
        }

        if (index == self->state.data.size()) {
          std::cout << "count path task finish." << std::endl;
          self->send(out_data, self->state.data[index - 1]);
          return;
        }

        self->state.index = index;
        self->state.current_result.clear();

        auto sender =
            caf::actor_cast<caf::strong_actor_ptr>(self->current_sender());
        auto self_actor = caf::actor_cast<caf::actor>(std::move(sender));

        for (int i = 0; i < index + 1; i++) {
          if (0 == i) {
            std::cout << "send add task:" << self->state.data[index - 1][0]
                      << ", " << self->state.data[index][0] << ", " << i
                      << std::endl;
            //            self->send(worker, self->state.data[index - 1][0],
            //                       self->state.data[index][0], i, self_actor);

            self->request(worker, caf::infinite, self->state.data[index - 1][0],
                          self->state.data[index][0], i, self_actor)
                .then([=](int) {
                  std::cout
                      << "add task complete:" << self->state.data[index - 1][0]
                      << ", " << self->state.data[index][0] << ", " << i
                      << std::endl;
                });

          } else if (i == index) {
            std::cout << "send add task:" << self->state.data[index - 1][i - 1]
                      << ", " << self->state.data[index][i] << ", " << i
                      << std::endl;
            //            self->send(worker, self->state.data[index - 1][i - 1],
            //                       self->state.data[index][i], i, self_actor);

            self->request(worker, caf::infinite,
                          self->state.data[index - 1][i - 1],
                          self->state.data[index][i], i, self_actor)
                .then([=](int) {
                  std::cout
                      << "add task complete:" << self->state.data[index - 1][0]
                      << ", " << self->state.data[index][0] << ", " << i
                      << std::endl;
                });

          } else {
            std::cout << "send add task:" << self->state.data[index - 1][i - 1]
                      << ", " << self->state.data[index - 1][i] << ", "
                      << self->state.data[index][i] << ", " << i << std::endl;
            //            self->send(worker, self->state.data[index - 1][i - 1],
            //                       self->state.data[index - 1][i],
            //                       self->state.data[index][i], i, self_actor);
            self->request(worker, caf::infinite,
                          self->state.data[index - 1][i - 1],
                          self->state.data[index - 1][i],
                          self->state.data[index][i], i, self_actor)
                .then([=](int) {
                  std::cout << "add task complete:"
                            << self->state.data[index - 1][i - 1] << ", "
                            << self->state.data[index - 1][i] << ", "
                            << self->state.data[index][i] << ", " << i
                            << std::endl;
                });
          }
        }
      },
      [=](int result, int id) {
        std::cout << ">>> receive result: " << result << ", id: " << id
                  << std::endl;
        if (0 == self->state.current_result.count(id)) {
          self->state.current_result.emplace(id, result);
        }
        if ((self->state.index + 1) == self->state.current_result.size()) {
          self->send(self, self->state.index + 1);
        }
      }};
}
