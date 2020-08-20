/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <climits>
#include <condition_variable>
#include <sstream>
#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>
#include <caf/openssl/all.hpp>

#include "./yanghui_config.h"
#include "./yanghui_simple_actor.h"
#include <cdcf/all.h>
#include "include/actor_union_count_cluster.h"
#include "include/cdcf_spawn.h"
#include "include/router_pool_count_cluster.h"
#include "include/simple_counter.h"
#include "include/yanghui_actor.h"
#include "include/yanghui_demo_calculator.h"
#include "include/yanghui_io.h"
#include "include/yanghui_server.h"
#include "include/yanghui_with_priority.h"

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

void SmartWorkerStart(caf::actor_system& system, const config& cfg) {
  YanghuiIO yanghui_io(cfg);

  auto actor1 = system.spawn<typed_calculator>();

  auto actor_port = yanghui_io.publish(caf::actor_cast<caf::actor>(actor1),
                                       k_yanghui_work_port1, nullptr, true);
  if (!actor_port) {
    std::cout << "publish actor1 failed, error: "
              << system.render(actor_port.error()) << std::endl;
    exit(1);
  }
  std::cout << "worker start at port:" << k_yanghui_work_port1 << std::endl;

  auto actor2 = system.spawn<typed_calculator>();
  actor_port = yanghui_io.publish(caf::actor_cast<caf::actor>(actor2),
                                  k_yanghui_work_port2, nullptr, true);
  if (!actor_port) {
    std::cout << "publish actor2 failed, error: "
              << system.render(actor_port.error()) << std::endl;
    exit(1);
  }
  std::cout << "worker start at port:" << k_yanghui_work_port2 << std::endl;

  auto actor3 = system.spawn<typed_calculator>();
  actor_port = yanghui_io.publish(caf::actor_cast<caf::actor>(actor3),
                                  k_yanghui_work_port3, nullptr, true);
  if (!actor_port) {
    std::cout << "publish actor3 failed, error: "
              << system.render(actor_port.error()) << std::endl;
    exit(1);
  }
  std::cout << "worker start at port:" << k_yanghui_work_port3 << std::endl;

  auto actor_for_load_balance_demo =
      system.spawn(simple_counter_add_load, cfg.worker_load);
  // Todo: 错误处理
  yanghui_io.publish(caf::actor_cast<caf::actor>(actor_for_load_balance_demo),
                     k_yanghui_work_port4);
  std::cout << "load balance worker start at port:" << k_yanghui_work_port4
            << ", worker_load:" << cfg.worker_load << std::endl;

  cdcf::ActorStatusMonitor actor_status_monitor(system);
  cdcf::ActorStatusServiceGrpcImpl actor_status_service(system,
                                                        actor_status_monitor);

  auto cdcf_spawn = system.spawn<CdcfSpawn>(&actor_status_monitor);

  actor_port = yanghui_io.publish(cdcf_spawn, cfg.worker_port, nullptr, true);

  if (!actor_port) {
    std::cout << "publish cdcf_spawn failed, error: "
              << system.render(actor_port.error()) << std::endl;
    exit(1);
  }

  auto form_actor1 = caf::actor_cast<caf::actor>(actor1);
  auto form_actor2 = caf::actor_cast<caf::actor>(actor2);
  auto form_actor3 = caf::actor_cast<caf::actor>(actor3);

  actor_status_monitor.RegisterActor(form_actor1, "calculator1",
                                     "a actor can calculate for yanghui.");
  actor_status_monitor.RegisterActor(form_actor2, "calculator2",
                                     "a actor can calculate for yanghui.");
  actor_status_monitor.RegisterActor(form_actor3, "calculator3",
                                     "a actor can calculate for yanghui.");
  actor_status_monitor.RegisterActor(
      actor_for_load_balance_demo, "calculator for load balance",
      "a actor can calculate for load balance yanghui.");

  std::cout << "yanghui server ready to work, press 'q' to stop." << std::endl;
  actor_status_service.Run();
  cdcf::cluster::Cluster::GetInstance()->NotifyReady();

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
  self->send_exit(cdcf_spawn, caf::exit_reason::user_shutdown);
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

std::vector<std::vector<int>> kYanghuiData3 = {
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
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 3},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2, 9, 4},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2, 9, 4, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2, 9, 4, 2, 3},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6, 4, 2, 9, 4, 7, 8, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1,
     6, 8, 7, 7, 6, 4, 2, 9, 4, 7, 8, 1, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6,
     8, 7, 7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 5},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6,
     8, 7, 7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8,
     7, 7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8,
     7, 7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 9, 5},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7,
     7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7,
     7, 6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7,
     6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7, 3, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7,
     6, 4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7, 3, 6, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6,
     4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7, 3, 6, 9, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6,
     4, 2, 9, 4, 7, 8, 1, 4, 6, 7, 3, 4, 7, 3, 6, 9, 5, 7},
};

void printRet(int return_value) {
  printf("call actor return value: %d\n", return_value);
  // std::cout << "call actor return value:" << return_value << std::endl;
}

caf::behavior result_print_actor(caf::event_based_actor* self) {
  return {[=](int result) {
    std::cout << "load balance count yanghui complete, get result:" << result
              << std::endl;
  }};
}

caf::behavior load_balance_result_print_actor(
    caf::event_based_actor* self, const caf::strong_actor_ptr& destination) {
  return {[=](int result) {
    self->send(caf::actor_cast<caf::actor>(destination), result);
    std::cout << "load balance count yanghui complete, get result:" << result
              << std::endl;
  }};
}

void downMsgHandle(const caf::down_msg& downMsg,
                   const std::string& actor_description) {
  CDCF_LOGGER_WARN("=============actor monitor call===============");
  CDCF_LOGGER_WARN("down actor address:{}", caf::to_string(downMsg.source));
  CDCF_LOGGER_WARN("down actor description:", actor_description);
  CDCF_LOGGER_WARN("down reason:", caf::to_string(downMsg.reason));

  //  std::cout << std::endl;
  //  std::cout << "=============actor monitor call===============" <<
  //  std::endl; std::cout << "down actor address:" <<
  //  caf::to_string(downMsg.source)
  //            << std::endl;
  //  std::cout << "down actor description:" << actor_description << std::endl;
  //  std::cout << "down reason:" << caf::to_string(downMsg.reason) <<
  //  std::endl; std::cout << std::endl; std::cout << std::endl;
}

void dealSendErr(const caf::error& err) {
  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
}

caf::actor InitHighPriorityYanghuiActors(caf::actor_system& system,
                                         WorkerPool& worker_pool) {
  worker_pool.Init();

  auto yanghui_actor_normal_priority =
      system.spawn(yanghui_with_priority, &worker_pool, false);
  auto yanghui_actor_high_priority =
      system.spawn(yanghui_with_priority, &worker_pool, true);
  auto yanghui_job_dispatcher_actor =
      system.spawn(yanghui_job_dispatcher, yanghui_actor_normal_priority,
                   yanghui_actor_high_priority);

  return yanghui_job_dispatcher_actor;
}

std::string InputHostIp() {
  std::cout << "Please Input host IP : " << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  return std::move(dummy);
}

size_t InputSize() {
  std::cout << "Please Input size : " << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  return (size_t)std::stoi(dummy);
}

int LocalYanghuiJob(const std::vector<std::vector<int>>& yanghui_data) {
  int n = yanghui_data.size();
  int* temp_states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
  int* states = reinterpret_cast<int*>(malloc(sizeof(int) * n));

  states[0] = 1;
  states[0] = yanghui_data[0][0];
  int i, j, k, min_sum = std::numeric_limits<int>::max();
  for (i = 1; i < n; i++) {
    for (j = 0; j < i + 1; j++) {
      if (j == 0) {
        temp_states[0] = states[0] + yanghui_data[i][j];
      } else if (j == i) {
        temp_states[j] = states[j - 1] + yanghui_data[i][j];
      } else {
        temp_states[j] =
            std::min(states[j - 1], states[j]) + yanghui_data[i][j];
      }
    }

    for (k = 0; k < i + 1; k++) {
      states[k] = temp_states[k];
    }
  }

  for (j = 0; j < n; j++) {
    if (states[j] < min_sum) min_sum = states[j];
  }

  free(temp_states);
  free(states);
  return min_sum;
}

void PublishActor(caf::actor_system& system, caf::actor actor, uint16_t port) {
  auto actual_port = system.middleman().publish(actor, port, nullptr, true);
  int retry_limit = 3;
  while (!actual_port && retry_limit > 0) {
    CDCF_LOGGER_DEBUG("retry publishing actor to port {} in 3 seconds", port);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    actual_port = system.middleman().publish(actor, port, nullptr, true);
    retry_limit--;
  }

  if (!actual_port) {
    CDCF_LOGGER_INFO("publish port failed: {}, error: {}", port,
                     caf::to_string(actual_port.error()));
  } else {
    CDCF_LOGGER_INFO("publish port succeeded: {}", port);
  }
}

void SmartRootStart(caf::actor_system& system, const config& cfg) {
  YanghuiIO yanghui_io(cfg);

  cdcf::ActorStatusMonitor actor_status_monitor(system);
  cdcf::ActorStatusServiceGrpcImpl actor_status_service(system,
                                                        actor_status_monitor);

  // router pool
  std::string routee_name = "calculator";
  auto routee_args = caf::make_message();
  auto routee_ifs = system.message_types<calculator>();
  auto policy = caf::actor_pool::round_robin();
  size_t default_size = 3;
  auto pool_cluster = new RouterPoolCountCluster(
      cfg.root_host, system, cfg.node_keeper_host, cfg.node_keeper_port,
      cfg.worker_port, routee_name, routee_args, routee_ifs, default_size,
      policy, yanghui_io.IsUseSSL());
  pool_cluster->InitWorkerNodes();
  auto pool_actor = system.spawn(yanghui, pool_cluster);
  std::cout << "pool_actor for router pool job spawned with id: "
            << pool_actor.id() << std::endl;
  actor_status_monitor.RegisterActor(
      pool_actor, "Yanghui",
      "a actor can count yanghui triangle using pool cluster.");

  auto pool_supervisor = system.spawn<cdcf::ActorMonitor>(downMsgHandle);
  cdcf::SetMonitor(pool_supervisor, pool_actor, "worker actor for testing");

  cdcf::ActorGuard pool_guard(
      pool_actor,
      [&](std::atomic<bool>& active) {
        active = true;
        auto new_yanghui = system.spawn(yanghui, pool_cluster);
        CDCF_LOGGER_ERROR(
            "yanghui actor for pool cluster failed. spawn new one: {}",
            caf::to_string(new_yanghui.address()));
        actor_status_monitor.RegisterActor(
            new_yanghui, "Yanghui",
            "a actor can count yanghui triangle using pool cluster.");
        // SetMonitor(supervisor, yanghui_actor, "worker actor for testing");
        return new_yanghui;
      },
      system);

  //  actor_union_count_cluster counter(cfg.root_host, system,
  //  cfg.node_keeper_port,
  //                                 cfg.worker_port);

  CountCluster* count_cluster;

  CDCF_LOGGER_INFO("Actor system log, hello world, I'm root.");

  auto* actor_cluster = new ActorUnionCountCluster(
      cfg.root_host, system, cfg.node_keeper_host, cfg.node_keeper_port,
      cfg.worker_port, yanghui_io);

  count_cluster = actor_cluster;

  count_cluster->InitWorkerNodes();

  // local test
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port1);
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port2);
  //  counter.AddWorkerNode("localhost", k_yanghui_work_port3);

  // counter.AddWorkerNode("localhost");
  // count_cluster->AddWorkerNode("localhost");

  auto yanghui_actor = system.spawn(yanghui, count_cluster);
  std::cout << "yanghui_actor for standard job spawned with id: "
            << yanghui_actor.id() << std::endl;

  actor_status_monitor.RegisterActor(
      yanghui_actor, "Yanghui",
      "a actor can count yanghui triangle using count cluster.");

  std::cout << "yanghui server ready to work, press 'n', 'p', 'b' or 'e' to "
               "go, 'q' to stop"
            << std::endl;

  auto supervisor = system.spawn<cdcf::ActorMonitor>(downMsgHandle);
  cdcf::SetMonitor(supervisor, yanghui_actor, "worker actor for testing");

  cdcf::ActorGuard actor_guard(
      yanghui_actor,
      [&](std::atomic<bool>& active) {
        active = true;
        auto new_yanghui = system.spawn(yanghui, count_cluster);
        CDCF_LOGGER_ERROR(
            "yanghui actor for count cluster failed. spawn new one: {}",
            caf::to_string(new_yanghui.address()));
        actor_status_monitor.RegisterActor(
            new_yanghui, "Yanghui",
            "a actor can count yanghui triangle using countcluster.");
        // SetMonitor(supervisor, yanghui_actor, "worker actor for testing");
        return new_yanghui;
      },
      system);

  auto yanghui_load_balance_result = system.spawn(result_print_actor);
  auto yanghui_load_balance_get_min =
      system.spawn(yanghui_get_final_result, actor_cluster->load_balance_,
                   yanghui_load_balance_result);

  auto yanghui_load_balance_count_path =
      system.spawn(yanghui_count_path, actor_cluster->load_balance_,
                   yanghui_load_balance_get_min);

  actor_status_service.Run();
  cdcf::cluster::Cluster::GetInstance()->NotifyReady();

  WorkerPool worker_pool(system, cfg.root_host, cfg.worker_port, yanghui_io);

  auto yanghui_job_dispatcher_actor =
      InitHighPriorityYanghuiActors(system, worker_pool);
  std::cout << "yanghui_job_dispatcher_actor spawned with id: "
            << yanghui_job_dispatcher_actor.id() << std::endl;

  auto yanghui_standard_job_actor =
      system.spawn(yanghui_standard_job_actor_fun, &actor_guard);
  std::cout << "yanghui_standard_job_actor spawned with id: "
            << yanghui_standard_job_actor.id() << std::endl;

  auto yanghui_priority_job_actor =
      system.spawn(yanghui_priority_job_actor_fun, &worker_pool,
                   yanghui_job_dispatcher_actor);
  std::cout << "yanghui_priority_job_actor spawned with id: "
            << yanghui_priority_job_actor.id() << std::endl;

  auto yanghui_load_balance_job_actor = system.spawn(
      yanghui_load_balance_job_actor_fun, yanghui_load_balance_count_path,
      yanghui_load_balance_get_min);

  auto yanghui_router_pool_job_actor =
      system.spawn(yanghui_router_pool_job_actor_fun, &pool_guard);
  std::cout << "yanghui_router_pool_job_actor spawned with id: "
            << yanghui_router_pool_job_actor.id() << std::endl;

  PublishActor(system, yanghui_standard_job_actor, yanghui_job1_port);
  PublishActor(system, yanghui_priority_job_actor, yanghui_job2_port);
  PublishActor(system, yanghui_load_balance_job_actor, yanghui_job3_port);
  PublishActor(system, yanghui_router_pool_job_actor, yanghui_job4_port);

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
      actor_guard.SendAndReceive(printRet, dealSendErr, kYanghuiData2);
      continue;
    }

    if (dummy == "b") {
      std::cout << "start load balance count." << std::endl;
      //  caf::anon_send(yanghui_load_balance_count_path, kYanghuiData2);
      caf::scoped_actor self{system};
      YanghuiData yanghui_data;
      yanghui_data.data = kYanghuiData2;
      self->send(yanghui_load_balance_job_actor, yanghui_data);
      self->receive(
          [&self](bool status, int result) {
            caf::aout(self)
                << "load balance count yanghui complete, get final result:"
                << result << std::endl;
          });
      continue;
    }

    if (dummy == "e") {
      std::cout << "start count." << std::endl;
      // self->send(yanghui_actor, kYanghuiData2);
      actor_guard.SendAndReceive(printRet, dealSendErr, "quit");

      continue;
    }

    if (dummy == "p") {
      std::cout << "start yanghui calculation with priority." << std::endl;
      while (true) {
        std::cout << "waiting for worker" << std::endl;
        if (!worker_pool.IsEmpty()) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      caf::scoped_actor self{system};

      self->send(yanghui_job_dispatcher_actor, kYanghuiData2);
      self->receive(
          [=](std::vector<std::pair<bool, int>> result) {
            for (const auto& pair : result) {
              if (pair.first) {
                std::cout << "high priority with final result: " << pair.second
                          << std::endl;
              } else {
                std::cout << "normal priority with final result: "
                          << pair.second << std::endl;
              }
            }
          },
          [&](caf::error error) {
            std::cout << "Receive yanghui priority result error: "
                      << system.render(error) << std::endl;
          });

      continue;
    }

    if (dummy == "pool n") {
      std::cout << "pool start count." << std::endl;
      pool_guard.SendAndReceive(printRet, dealSendErr, kYanghuiData2);
    }

    if (dummy == "pool get nodes") {
      std::cout << pool_cluster->NodeList() << std::endl;
    }

    if (dummy == "pool remove node") {
      std::string host = InputHostIp();
      std::cout << pool_cluster->RemoveNode(host, cfg.worker_port) << std::endl;
    }

    if (dummy == "pool add node") {
      std::string host = InputHostIp();
      std::cout << pool_cluster->AddNode(host, cfg.worker_port) << std::endl;
    }

    if (dummy == "pool get size") {
      std::cout << pool_cluster->GetPoolSize() << std::endl;
    }

    if (dummy == "pool get size by ip") {
      std::string host = InputHostIp();
      std::cout << pool_cluster->GetPoolSize(host, cfg.worker_port)
                << std::endl;
    }

    if (dummy == "pool change size") {
      auto size = InputSize();
      std::cout << pool_cluster->ChangePoolSize(size) << std::endl;
    }

    if (dummy == "pool change size by ip") {
      auto size = InputSize();
      std::string host = InputHostIp();
      std::cout << pool_cluster->ChangePoolSize(size, host, cfg.worker_port)
                << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

int AddYanghuiJob(
    caf::actor_system& system, std::string host,
    std::vector<caf::actor>& yanghui_jobs,
    std::unordered_map<uint16_t, std::string>& job_actor_id_to_name) {
  auto yanghui_job1_actor =
      system.middleman().remote_actor(host, yanghui_job1_port);
  if (!yanghui_job1_actor) {
    std::cerr << "unable to connect to yanghui_job1_actor: "
              << to_string(yanghui_job1_actor.error()) << '\n';
    return 1;
  }
  job_actor_id_to_name[yanghui_job1_actor->id()] = yanghui_job1_name;

  auto yanghui_job2_actor =
      system.middleman().remote_actor(host, yanghui_job2_port);
  if (!yanghui_job2_actor) {
    std::cerr << "unable to connect to yanghui_job2_actor: "
              << to_string(yanghui_job2_actor.error()) << '\n';
    return 1;
  }
  job_actor_id_to_name[yanghui_job2_actor->id()] = yanghui_job2_name;

  auto yanghui_job3_actor =
      system.middleman().remote_actor(host, yanghui_job3_port);
  if (!yanghui_job3_actor) {
    std::cerr << "unable to connect to yanghui_job3_actor: "
              << to_string(yanghui_job3_actor.error()) << '\n';
    return 1;
  }
  job_actor_id_to_name[yanghui_job3_actor->id()] = yanghui_job3_name;

  auto yanghui_job4_actor =
      system.middleman().remote_actor(host, yanghui_job4_port);
  if (!yanghui_job4_actor) {
    std::cerr << "unable to connect to yanghui_job4_actor: "
              << to_string(yanghui_job4_actor.error()) << '\n';
    return 1;
  }
  job_actor_id_to_name[yanghui_job4_actor->id()] = yanghui_job4_name;

  yanghui_jobs.push_back(*yanghui_job1_actor);
  yanghui_jobs.push_back(*yanghui_job2_actor);
  yanghui_jobs.push_back(*yanghui_job3_actor);
  yanghui_jobs.push_back(*yanghui_job4_actor);

  std::cout << "yanghui_job1_actor: " << yanghui_job1_actor->id() << std::endl;
  std::cout << "yanghui_job2_actor: " << yanghui_job2_actor->id() << std::endl;
  std::cout << "yanghui_job3_actor: " << yanghui_job3_actor->id() << std::endl;
  std::cout << "yanghui_job4_actor: " << yanghui_job4_actor->id() << std::endl;

  return 0;
}

bool SendJobAndCheckResult(
    caf::actor_system& system, caf::scoped_actor& self, caf::actor job_actor,
    YanghuiData& yanghui_data, int result_to_check,
    std::unordered_map<uint16_t, std::string>& job_actor_id_to_name) {
  bool running_status_normal = true;

  self->send(job_actor, yanghui_data);
  auto now = std::chrono::high_resolution_clock::now();
  auto time_out = now + std::chrono::seconds(300);
  if (self->await_data(time_out)) {
    self->receive(
        [&](bool status, int result) {
          if (!status) {
            running_status_normal = false;
            CDCF_LOGGER_ERROR("Yanghui Test: {} failed",
                              job_actor_id_to_name[job_actor.id()]);
          } else {
            if (result != result_to_check) {
              running_status_normal = false;
              CDCF_LOGGER_ERROR("Yanghui Test: {} result inconsistent",
                                job_actor_id_to_name[job_actor.id()]);
            } else {
              CDCF_LOGGER_INFO("Yanghui Test: {} succeeded with result {}",
                               job_actor_id_to_name[job_actor.id()], result);
            }
          }
        },
        [&](caf::error& err) {
          running_status_normal = false;
          CDCF_LOGGER_ERROR("Yanghui Test: {} failed, {}",
                            job_actor_id_to_name[job_actor.id()],
                            system.render(err));
        });
  } else {
    CDCF_LOGGER_ERROR("Yanghui Test: {} timeout",
                      job_actor_id_to_name[job_actor.id()]);
    running_status_normal = false;
  }

  return running_status_normal;
}

void SillyClientStart(caf::actor_system& system, const config& cfg) {
  std::cout << "wait 10 seconds before try to connect to yanghui job"
            << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::vector<caf::actor> yanghui_jobs;
  std::unordered_map<uint16_t, std::string> job_actor_id_to_name;

  int try_limit = 5;
  int error =
      AddYanghuiJob(system, cfg.root_host, yanghui_jobs, job_actor_id_to_name);
  while (error != 0 && try_limit > 0) {
    CDCF_LOGGER_DEBUG(
        "Yanghui Test: retry connecting to yanghui job in 10 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    error = AddYanghuiJob(system, cfg.root_host, yanghui_jobs,
                          job_actor_id_to_name);
    try_limit--;
  }

  if (error) {
    CDCF_LOGGER_ERROR("Yanghui Test: connection to yanghui job failed");
    return;
  }

  caf::scoped_actor self{system};

  std::vector<YanghuiData> yanghui_data;
  yanghui_data.emplace_back(kYanghuiData2);
  yanghui_data.emplace_back(kYanghuiData3);

  std::vector<int> yanghui_job_result;
  yanghui_job_result.emplace_back(LocalYanghuiJob(kYanghuiData2));
  yanghui_job_result.emplace_back(LocalYanghuiJob(kYanghuiData3));

  int successful_job_counter = 0;
  bool running_status_normal = true;
  while (running_status_normal) {
    for (int j = 0; j < yanghui_data.size(); j++) {
      CDCF_LOGGER_INFO(
          "Yanghui Test: new round starts, total successful job count is {}",
          successful_job_counter);
      for (int i = 0; i < yanghui_jobs.size(); i++) {
        std::cout << "sending job to yanghui_job " << i << std::endl;
        running_status_normal = SendJobAndCheckResult(
            system, self, yanghui_jobs[i], yanghui_data[j],
            yanghui_job_result[j], job_actor_id_to_name);
        successful_job_counter++;
        if (!running_status_normal) {
          break;
        }
      }
      if (!running_status_normal) {
        break;
      }
    }
  }
}

void caf_main(caf::actor_system& system, const config& cfg) {
  cdcf::Logger::Init(cfg);
  std::cout << "role: " << cfg.role_ << std::endl;
  CDCF_LOGGER_DEBUG("I am {}", cfg.role_);
  if (cfg.role_ == "root") {
    SmartRootStart(system, cfg);
  } else if (cfg.role_ == "client") {
    SillyClientStart(system, cfg);
  } else {
    SmartWorkerStart(system, cfg);
  }
}

CAF_MAIN(caf::io::middleman, caf::openssl::manager)
