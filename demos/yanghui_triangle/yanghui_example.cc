/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "./yanghui_config.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

std::vector<std::vector<int> > kYanghuiData = {
    {5}, {7, 8}, {2, 1, 4}, {4, 2, 6, 1}, {2, 7, 3, 4, 5}, {2, 3, 7, 6, 8, 3}};

struct YanghuiState {
  int index = 0;
  std::map<int, int> current_result;
  std::vector<std::vector<int> > data;
};

struct GetMinState {
  int count = 0;
  std::map<int, int> current_result;
};

caf::behavior getMin(caf::stateful_actor<GetMinState>* self,
                     const caf::actor worker) {
  const int batch = 3;
  return {[=](const std::vector<int>& data) {
            int len = data.size();
            if (1 == len) {
              std::cout << "final result:" << data[0] << std::endl;
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
              self->send(worker, send_data);
              // self->send(worker_group, 1, 2, 3);
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
          },
          [=](const caf::group& what) {
            for (const auto& g : self->joined_groups()) {
              std::cout << "*** leave " << to_string(g) << std::endl;
              self->leave(g);
            }

            self->join(what);
            std::cout << "compare joined a group:" << to_string(what)
                      << std::endl;
          }};
}

caf::behavior yanghui(caf::stateful_actor<YanghuiState>* self,
                      const caf::actor worker, const caf::actor compare) {
  return {[=](const std::vector<std::vector<int> >& data) {
            const int len = data.size();
            self->state.data = data;
            self->state.current_result[0] = data[0][0];
            self->send(self, 1);
          },
          [=](int index) {
            for (auto mapValue : self->state.current_result) {
              self->state.data[index - 1][mapValue.first] = mapValue.second;
            }

            if (index == self->state.data.size()) {
              std::cout << "count task finish." << std::endl;
              self->send(compare, self->state.data[index - 1]);
              return;
            }

            self->state.index = index;
            self->state.current_result.clear();

            for (int i = 0; i < index + 1; i++) {
              if (0 == i) {
                self->send(worker, self->state.data[index - 1][0],
                           self->state.data[index][0], i);
              } else if (i == index) {
                self->send(worker, self->state.data[index - 1][i - 1],
                           self->state.data[index][i], i);
              } else {
                self->send(worker, self->state.data[index - 1][i - 1],
                           self->state.data[index - 1][i],
                           self->state.data[index][i], i);
              }
            }
          },
          [=](int result, int id) {
            if (0 == self->state.current_result.count(id)) {
              self->state.current_result.emplace(id, result);
            }
            if ((self->state.index + 1) == self->state.current_result.size()) {
              self->send(self, self->state.index + 1);
            }
          },
          [=](const caf::group& what) {
            for (const auto& g : self->joined_groups()) {
              std::cout << "*** leave " << to_string(g) << std::endl;
              self->leave(g);
            }

            self->join(what);
            std::cout << "yanghui joined a group:" << to_string(what)
                      << std::endl;
          }};
}

void caf_main(caf::actor_system& system, const config& cfg) {
  auto result_group_exp = system.groups().get("remote", cfg.count_result_group);
  if (!result_group_exp) {
    std::cerr << "failed to get count result group: " << cfg.count_result_group
              << std::endl;
    return;
  }

  auto compare_group_exp = system.groups().get("remote", cfg.compare_group);
  if (!compare_group_exp) {
    std::cerr << "failed to get compare result group: " << cfg.compare_group
              << std::endl;
    return;
  }

  auto result_group = std::move(*result_group_exp);
  auto compare_group = std::move(*compare_group_exp);

  caf::scoped_actor self{system};
  caf::scoped_execution_unit context(&system);
  auto pool = caf::actor_pool::make(&context, caf::actor_pool::round_robin());

  auto worker1_exp = system.middleman().remote_actor("localhost", 55001);
  if (!worker1_exp) {
    std::cout << "connect remote actor failed. host:localhost"
              << ", port:" << 51563 << std::endl;
  }

  auto worker1 = std::move(*worker1_exp);

  auto worker2_exp = system.middleman().remote_actor("localhost", 51566);
  auto worker2 = std::move(*worker2_exp);

  self->send(pool, caf::sys_atom::value, caf::put_atom::value, worker1);
  self->send(pool, caf::sys_atom::value, caf::put_atom::value, worker2);

  auto min_actor = system.spawn(getMin, pool);
  auto min_actor_fun = make_function_view(min_actor);
  min_actor_fun(compare_group);

  auto yanghui_actor = system.spawn(yanghui, pool, min_actor);
  auto yanghui_actor_fun = make_function_view(yanghui_actor);
  yanghui_actor_fun(result_group);

  self->send(yanghui_actor, kYanghuiData);
}

CAF_MAIN(caf::io::middleman)
