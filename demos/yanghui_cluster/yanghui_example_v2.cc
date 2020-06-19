/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <actor_system/cluster.h>

#include <climits>
#include <condition_variable>
#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "../../actor_fault_tolerance/include/actor_union.h"
#include "../../actor_monitor/include/actor_monitor.h"
#include "../../actor_system/include/actor_status_service_grpc_impl.h"
#include "./yanghui_config.h"

caf::actor StartWorker(caf::actor_system& system, const caf::node_id& nid,
                       const std::string& name, caf::message args,
                       std::chrono::seconds timeout, bool& active) {
  auto worker = system.middleman().remote_spawn<calculator>(
      nid, name, std::move(args), timeout);
  if (!worker) {
    std::cerr << "*** remote spawn failed: " << system.render(worker.error())
              << std::endl;

    active = false;
    return caf::actor{};
  }

  auto ret_actor = caf::actor_cast<caf::actor>(std::move(*worker));
  active = true;

  return ret_actor;
}

const uint16_t k_yanghui_work_port1 = 55001;
const uint16_t k_yanghui_work_port2 = 55002;
const uint16_t k_yanghui_work_port3 = 55003;

class CountCluster : public actor_system::cluster::Observer {
 public:
  virtual void AddWorkerNode(const std::string& host) = 0;
  virtual int AddNumber(int a, int b, int& result) = 0;
  virtual int Compare(std::vector<int> numbers, int& min) = 0;
  std::string host_;

  explicit CountCluster(std::string host) : host_(std::move(host)) {
    auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
    InitWorkerNodes(members, host_);
    actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
  }

  virtual ~CountCluster() {
    actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
  }

  void InitWorkerNodes(
      const std::vector<actor_system::cluster::Member>& members,
      const std::string& host) {
    std::cout << "self, host: " << host << std::endl;
    std::cout << "members size:" << members.size() << std::endl;
    for (auto& m : members) {
      std::cout << "member, host: " << m.host << std::endl;
      if (m.host == host) {
        continue;
      }
      std::cout << "add worker, host: " << m.host << std::endl;
      AddWorkerNode(m.host);
    }
  }

  void Update(const actor_system::cluster::Event& event) override {
    std::cout << "=======get update event, host:" << event.member.host
              << std::endl;

    if (event.member.host != host_) {
      if (event.member.status == event.member.Up) {
        // std::this_thread::sleep_for(std::chrono::seconds(2));
        AddWorkerNode(event.member.host);
      } else {
        // Todo(Yujia.Li): resource leak
        std::cout << "detect worker node down, host:" << event.member.host
                  << " port:" << event.member.port << std::endl;
      }
    }
  }
};

class BalanceCountCluster : public CountCluster {
 public:
  caf::actor_system& system_;
  caf::scoped_execution_unit context_;
  std::string host_;
  caf::actor counter_;
  caf::scoped_actor sender_actor_;

  BalanceCountCluster(const std::string& host, caf::actor_system& system)
      : CountCluster(host),
        system_(system),
        context_(&system_),
        host_(host),
        sender_actor_(system_) {
    auto policy = cdcf::load_balancer::policy::MinLoad(1);
    counter_ = cdcf::load_balancer::Router::Make(&context_, std::move(policy));
  }

  virtual ~BalanceCountCluster() {}

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port) {
    auto worker_actor = system_.middleman().remote_actor(host, port);
    if (!worker_actor) {
      std::cout << "connect remote actor failed. host:" << host
                << ", port:" << port << std::endl;
    }

    caf::anon_send(counter_, caf::sys_atom::value, caf::put_atom::value,
                   *worker_actor);

    std::cout << "=======add pool member host:" << host << ", port:" << port
              << std::endl;
  }

  void AddWorkerNode(const std::string& host) override {
    AddWorkerNodeWithPort(host, k_yanghui_work_port1);
    AddWorkerNodeWithPort(host, k_yanghui_work_port2);
    AddWorkerNodeWithPort(host, k_yanghui_work_port3);
  }

  int AddNumber(int a, int b, int& result) override {
    int error = 0;
    std::promise<int> promise;

    std::cout << "start add task input:" << a << ", " << b << std::endl;

    sender_actor_->request(counter_, std::chrono::seconds(5), a, b)
        .receive([&](int ret) { promise.set_value(ret); },
                 [&](const caf::error& err) {
                   std::cout
                       << "send add request get err: " << caf::to_string(err)
                       << "input data: a=" << a << ", b=" << b << std::endl;
                   error = 1;
                 });

    result = promise.get_future().get();

    std::cout << "get result:" << result << std::endl;
    return error;
  }

  int Compare(std::vector<int> numbers, int& min) override {
    int error = 0;
    std::promise<int> promise;
    std::cout << "start compare task. input data:" << std::endl;

    for (int p : numbers) {
      std::cout << p << " ";
    }

    std::cout << std::endl;

    NumberCompareData send_data;
    send_data.numbers = numbers;

    sender_actor_->request(counter_, std::chrono::seconds(5), send_data)
        .receive(
            [&](int ret) {
              min = ret;
              promise.set_value(ret);
            },
            [&](const caf::error& err) {
              std::cout << "send add request get err: " << caf::to_string(err)
                        << "input data:";
              for (auto num : numbers) {
                std::cout << num << ", ";
              }
              std::cout << std::endl;
              error = 1;
            });

    min = promise.get_future().get();
    std::cout << "get min:" << min << std::endl;

    return error;
  }
};

class ActorUnionCountCluster : public CountCluster {
 public:
  explicit ActorUnionCountCluster(std::string host, caf::actor_system& system,
                                  uint16_t port, uint16_t worker_port)
      : CountCluster(host),
        host_(std::move(host)),
        system_(system),
        port_(port),
        worker_port_(worker_port),
        counter_(system, caf::actor_pool::round_robin()) {}

  virtual ~ActorUnionCountCluster() {}

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port) {
    auto worker_actor = system_.middleman().remote_actor(host, port);
    if (!worker_actor) {
      std::cout << "connect remote actor failed. host:" << host
                << ", port:" << port << std::endl;
    }

    counter_.AddActor(*worker_actor);

    std::cout << "=======add pool member host:" << host << ", port:" << port
              << std::endl;
  }

  void AddWorkerNode(const std::string& host) override {
    AddWorkerNodeWithPort(host, k_yanghui_work_port1);
    AddWorkerNodeWithPort(host, k_yanghui_work_port2);
    AddWorkerNodeWithPort(host, k_yanghui_work_port3);
  }

  int AddNumber(int a, int b, int& result) override {
    int error = 0;
    std::promise<int> promise;

    std::cout << "start add task input:" << a << ", " << b << std::endl;

    counter_.SendAndReceive([&](int ret) { promise.set_value(ret); },
                            [&](const caf::error& err) { error = 1; }, a, b);

    result = promise.get_future().get();

    std::cout << "get result:" << result << std::endl;
    return error;
  }

  int Compare(std::vector<int> numbers, int& min) override {
    int error = 0;
    std::promise<int> promise;
    std::cout << "start compare task. input data:" << std::endl;

    for (int p : numbers) {
      std::cout << p << " ";
    }

    std::cout << std::endl;

    NumberCompareData send_data;
    send_data.numbers = numbers;

    counter_.SendAndReceive(
        [&](int ret) {
          min = ret;
          promise.set_value(ret);
        },
        [&](const caf::error& err) { error = 1; }, send_data);

    min = promise.get_future().get();
    std::cout << "get min:" << min << std::endl;

    return error;
  }

  caf::actor_system& system_;
  std::string host_;
  uint16_t port_;
  uint16_t worker_port_;
  ActorUnion counter_;
};

void SmartWorkerStart(caf::actor_system& system, const config& cfg) {
  auto actor1 = system.spawn<typed_calculator>();
  system.middleman().publish(caf::actor_cast<caf::actor>(actor1),
                             k_yanghui_work_port1);
  std::cout << "worker start at port:" << k_yanghui_work_port1 << std::endl;

  auto actor2 = system.spawn<typed_slow_calculator>();
  system.middleman().publish(caf::actor_cast<caf::actor>(actor2),
                             k_yanghui_work_port2);
  std::cout << "worker start at port:" << k_yanghui_work_port2 << std::endl;

  auto actor3 = system.spawn<typed_slow_calculator>();
  system.middleman().publish(caf::actor_cast<caf::actor>(actor3),
                             k_yanghui_work_port3);
  std::cout << "worker start at port:" << k_yanghui_work_port2 << std::endl;

  ActorStatusMonitor actor_status_monitor(system);
  ActorStatusServiceGprcImpl actor_status_service(system, actor_status_monitor);

  auto form_actor1 = caf::actor_cast<caf::actor>(actor1);
  auto form_actor2 = caf::actor_cast<caf::actor>(actor2);
  auto form_actor3 = caf::actor_cast<caf::actor>(actor3);

  actor_status_monitor.RegisterActor(form_actor1, "calculator1",
                                     "a actor can calculate for yanghui.");
  actor_status_monitor.RegisterActor(form_actor2, "calculator2",
                                     "a actor can calculate for yanghui.");
  actor_status_monitor.RegisterActor(form_actor3, "calculator3",
                                     "a actor can calculate for yanghui.");

  std::cout << "yanghui server ready to work, press 'q' to stop." << std::endl;
  actor_status_service.Run();

  // start compute
  while (true) {
    std::string dummy;
    std::getline(std::cin, dummy);
    if (dummy == "q") {
      std::cout << "stop work" << std::endl;
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  caf::scoped_actor self{system};

  self->send_exit(actor1, caf::exit_reason::user_shutdown);
  self->send_exit(actor2, caf::exit_reason::user_shutdown);
  self->send_exit(actor3, caf::exit_reason::user_shutdown);
}

caf::behavior yanghui(caf::event_based_actor* self, CountCluster* counter) {
  return {
      [=](const std::vector<std::vector<int>>& data) {
        int n = data.size();
        //        int temp_states[n];
        //        int states[n];
        int* temp_states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
        int* states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
        int error = 0;

        states[0] = 1;
        states[0] = data[0][0];
        int i, j, k, min_sum = INT_MAX;
        for (i = 1; i < n; i++) {
          for (j = 0; j < i + 1; j++) {
            if (j == 0) {
              // temp_states[0] = states[0] + data[i][j];
              error = counter->AddNumber(states[0], data[i][j], temp_states[0]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            } else if (j == i) {
              // temp_states[j] = states[j - 1] + data[i][j];
              error =
                  counter->AddNumber(states[j - 1], data[i][j], temp_states[j]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            } else {
              // temp_states[j] = std::min(states[j - 1], states[j]) +
              // data[i][j];
              error = counter->AddNumber(std::min(states[j - 1], states[j]),
                                         data[i][j], temp_states[j]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            }
          }

          for (k = 0; k < i + 1; k++) {
            states[k] = temp_states[k];
          }
        }

        //    for (j = 0; j < n; j++) {
        //      if (states[j] < min_sum) min_sum = states[j];
        //    }

        std::vector<int> states_vec(states, states + n);

        error = counter->Compare(states_vec, min_sum);
        if (0 != error) {
          caf::aout(self) << "cluster down, exit task" << std::endl;
          return INT_MAX;
        }

        caf::aout(self) << "yanghui triangle actor task complete, result: "
                        << min_sum << std::endl;
        free(temp_states);
        free(states);
        return min_sum;
      },
      [=](std::string&) {
        caf::aout(self) << "simulate get a critical error， yanghui actor quit."
                        << std::endl;
        self->quit();
        return 0;
      }};
}

std::vector<std::vector<int>> kYanghuiData2 = {
    {5},
    {7, 8},
    {2, 1, 4},
    {4, 2, 6, 1},
    {2, 7, 3, 4, 5},
    {2, 3, 7, 6, 8, 3},
    {2, 3, 4, 4, 2, 7, 7},
    {8, 4, 4, 3, 4, 5, 6, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6}};

void printRet(int return_value) {
  printf("call actor return value: %d\n", return_value);
  // std::cout << "call actor return value:" << return_value << std::endl;
}

void downMsgHandle(const caf::down_msg& downMsg,
                   const std::string& actor_description) {
  std::cout << std::endl;
  std::cout << "=============actor monitor call===============" << std::endl;
  std::cout << "down actor address:" << caf::to_string(downMsg.source)
            << std::endl;
  std::cout << "down actor description:" << actor_description << std::endl;
  std::cout << "down reason:" << caf::to_string(downMsg.reason) << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
}

void dealSendErr(const caf::error& err) {
  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
}

void SmartRootStart(caf::actor_system& system, const config& cfg) {
  //  ActorUnionCountCluster counter(cfg.root_host, system,
  //  cfg.node_keeper_port,
  //                                 cfg.worker_port);

  //  BalanceCountCluster balance_count_cluster(cfg.root_host, system);

  CountCluster* count_cluster;

  if (cfg.balance_mode) {
    count_cluster = new BalanceCountCluster(cfg.root_host, system);
  } else {
    count_cluster = new ActorUnionCountCluster(
        cfg.root_host, system, cfg.node_keeper_port, cfg.worker_port);
  }

  // local test
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port1);
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port2);
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port3);

  // counter.AddWorkerNode("localhost");
  count_cluster->AddWorkerNode("localhost");

  ActorStatusMonitor actor_status_monitor(system);
  ActorStatusServiceGprcImpl actor_status_service(system, actor_status_monitor);

  auto yanghui_actor = system.spawn(yanghui, count_cluster);
  actor_status_monitor.RegisterActor(yanghui_actor, "Yanghui",
                                     "a actor can count yanghui triangle.");

  std::cout << "yanghui server ready to work, press 'n' to go, 'q' to stop"
            << std::endl;

  auto supervisor = system.spawn<ActorMonitor>(downMsgHandle);
  SetMonitor(supervisor, yanghui_actor, "worker actor for testing");

  ActorGuard actor_guard(
      yanghui_actor,
      [&](std::atomic<bool>& active) {
        active = true;
        auto new_yanghui = system.spawn(yanghui, count_cluster);
        actor_status_monitor.RegisterActor(
            yanghui_actor, "Yanghui", "a actor can count yanghui triangle.");
        // SetMonitor(supervisor, yanghui_actor, "worker actor for testing");
        return new_yanghui;
      },
      system);

  actor_status_service.Run();

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
      // self->send(yanghui_actor, kYanghuiData2);
      for (int i = 0; i < 10; i++) {
        actor_guard.SendAndReceive(printRet, dealSendErr, kYanghuiData2);
      }

      continue;
    }

    if (dummy == "e") {
      std::cout << "start count." << std::endl;
      // self->send(yanghui_actor, kYanghuiData2);
      actor_guard.SendAndReceive(printRet, dealSendErr, "quit");

      continue;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void caf_main(caf::actor_system& system, const config& cfg) {
  if (cfg.root) {
    SmartRootStart(system, cfg);
  } else {
    SmartWorkerStart(system, cfg);
  }
}

CAF_MAIN(caf::io::middleman)
