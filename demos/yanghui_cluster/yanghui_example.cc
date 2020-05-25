/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <actor_system.h>

#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "./yanghui_config.h"

std::vector<std::vector<int>> kYanghuiData = {
    {5}, {7, 8}, {2, 1, 4}, {4, 2, 6, 1}, {2, 7, 3, 4, 5}, {2, 3, 7, 6, 8, 3}};

struct YanghuiState {
  int index = 0;
  std::map<int, int> current_result;
  std::vector<std::vector<int>> data;
};

struct GetMinState {
  int count = 0;
  std::map<int, int> current_result;
};

caf::behavior getMin(caf::stateful_actor<GetMinState>* self,
                     const caf::actor& worker) {
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
                      const caf::actor& worker, const caf::actor& compare) {
  return {[=](const std::vector<std::vector<int>>& data) {
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

class CountCluster : public actor_system::cluster::Observer {
 public:
  CountCluster(std::string host, caf::actor_system& system,
               caf::actor& worker_pool, uint16_t port, uint16_t worker_port)
      : host_(std::move(host)),
        system_(system),
        worker_pool_(worker_pool),
        port_(port),
        worker_port_(worker_port) {
    actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
  }
  ~CountCluster() {
    actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
  }

  void Update(const actor_system::cluster::Event& event) override {
    std::cout << "=======get update event, host:" << event.member.host
              << std::endl;

    if (event.member.host != host_) {
      if (event.member.status == event.member.Up) {
        // std::this_thread::sleep_for(std::chrono::seconds(2));
        AddWorkerNode(event.member.host, worker_port_);
      } else {
        std::cout << "detect worker node down, host:" << event.member.host
                  << " port:" << event.member.port << std::endl;
      }

    } else {
    }
  }

  void AddWorkerNode(const std::string& host, uint16_t port) {
    auto worker_exp = system_.middleman().remote_actor(host, port);
    auto worker = std::move(*worker_exp);
    caf::anon_send(worker_pool_, caf::sys_atom::value, caf::put_atom::value,
                   worker);
    std::cout << "=======add pool member host:" << host << std::endl;
  }

  caf::actor_system& system_;
  caf::actor& worker_pool_;
  std::string host_;
  uint16_t port_;
  uint16_t worker_port_;
};

// old worker
caf::behavior countAdd(caf::event_based_actor* self,
                       const caf::group& result_group,
                       const caf::group& compare_result_group) {
  return {[=](int a, int b, int id) {
            aout(self) << "receive:" << a << " " << b << " " << id << " "
                       << std::endl;
            anon_send(result_group, a + b, id);
          },
          [=](int a, int b, int c, int id) {
            aout(self) << "receive:" << a << " " << b << " " << c << " " << id
                       << " " << std::endl;
            anon_send(result_group, a < b ? a + c : b + c, id);
          },
          [=](NumberCompareData& a) {
            if (a.numbers.empty()) {
              return;
            }

            aout(self) << "receive:" << a << std::endl;

            int result = a.numbers[0];
            for (int num : a.numbers) {
              if (num < result) {
                result = num;
              }
            }

            anon_send(compare_result_group, result, a.index);
          }};
}

void WorkerStart(caf::actor_system& system, const config& cfg) {
  auto result_group_exp =
      system.groups().get("remote", cfg.kResultGroupName + "@" + cfg.host +
                                        ":" + std::to_string(cfg.port));
  if (!result_group_exp) {
    std::cerr << "failed to get count result group: "
              << cfg.kResultGroupName + "@" + cfg.host + ":" +
                     std::to_string(cfg.port)
              << std::endl;
    return;
  }

  auto compare_group_exp =
      system.groups().get("remote", cfg.kCompareGroupName + "@" + cfg.host +
                                        ":" + std::to_string(cfg.port));
  if (!compare_group_exp) {
    std::cerr << "failed to get compare result group: "
              << cfg.kCompareGroupName + "@" + cfg.host + ":" +
                     std::to_string(cfg.port)
              << std::endl;
    return;
  }

  auto result_group = std::move(*result_group_exp);
  auto compare_group = std::move(*compare_group_exp);
  auto worker_actor = system.spawn(countAdd, result_group, compare_group);

  auto expected_port = caf::io::publish(worker_actor, cfg.worker_port);
  if (!expected_port) {
    std::cerr << "*** publish failed: " << system.render(expected_port.error())
              << std::endl;
    return;
  }

  std::cout << "worker started at port:" << expected_port << std::endl
            << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  std::cout << "... cya" << std::endl;
}

static std::unique_ptr<CountCluster> router;

void RootStart(caf::actor_system& system, const config& cfg) {
  // start group server
  auto res = system.middleman().publish_local_groups(cfg.port);
  if (!res) {
    std::cerr << "*** publishing local groups failed: "
              << system.render(res.error()) << std::endl;
    return;
  } else {
    std::cout << "group server start at port:" << cfg.port << std::endl;
  }

  // start group
  auto result_group_exp =
      system.groups().get("remote", cfg.kResultGroupName + "@" + cfg.host +
                                        ":" + std::to_string(cfg.port));
  if (!result_group_exp) {
    std::cerr << "failed to get count result group: "
              << cfg.kResultGroupName + "@" + cfg.host + ":" +
                     std::to_string(cfg.port)
              << std::endl;
    return;
  }

  auto compare_group_exp =
      system.groups().get("remote", cfg.kCompareGroupName + "@" + cfg.host +
                                        ":" + std::to_string(cfg.port));
  if (!compare_group_exp) {
    std::cerr << "failed to get compare result group: "
              << cfg.kCompareGroupName + "@" + cfg.host + ":" +
                     std::to_string(cfg.port)
              << std::endl;
    return;
  }

  // start cluster
  auto result_group = std::move(*result_group_exp);
  auto compare_group = std::move(*compare_group_exp);

  caf::scoped_actor self{system};
  caf::scoped_execution_unit context(&system);
  auto pool = caf::actor_pool::make(&context, caf::actor_pool::round_robin());

  router = std::make_unique<CountCluster>(
      cfg.host, system, pool, cfg.node_keeper_port, cfg.worker_port);

  // start actor
  auto min_actor = system.spawn(getMin, pool);
  auto min_actor_fun = make_function_view(min_actor);
  min_actor_fun(compare_group);

  auto yanghui_actor = system.spawn(yanghui, pool, min_actor);
  auto yanghui_actor_fun = make_function_view(yanghui_actor);
  yanghui_actor_fun(result_group);

  // start compute
  while (true) {
    std::string dummy;
    std::getline(std::cin, dummy);
    if (dummy == "q") {
      std::cout << "stop work" << std::endl;
      break;
    }

    if (dummy == "n") {
      std::cout << "start count." << std::endl;
      self->send(yanghui_actor, kYanghuiData);
      continue;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void caf_main(caf::actor_system& system, const config& cfg) {
  if (cfg.root) {
    RootStart(system, cfg);
  } else {
    WorkerStart(system, cfg);
  }
}

CAF_MAIN(caf::io::middleman)
