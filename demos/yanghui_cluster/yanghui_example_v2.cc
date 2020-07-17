/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <actor_system/cluster.h>

#include <climits>
#include <condition_variable>
#include <sstream>
#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "../../actor_fault_tolerance/include/actor_union.h"
#include "../../actor_monitor/include/actor_monitor.h"
#include "../../actor_system/include/actor_status_service_grpc_impl.h"
#include "../../logger/include/logger.h"
#include "./yanghui_config.h"
#include "./yanghui_simple_actor.h"
#include "include/actor_union_count_cluster.h"
#include "include/balance_count_cluster.h"
#include "include/simple_counter.h"
#include "include/yanghui_actor.h"
#include "include/yanghui_demo_calculator.h"

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
  std::cout << "worker start at port:" << k_yanghui_work_port3 << std::endl;

  auto actor_for_load_balance_demo =
      system.spawn(simple_counter_add_load, cfg.worker_load);
  system.middleman().publish(
      caf::actor_cast<caf::actor>(actor_for_load_balance_demo),
      k_yanghui_work_port4);
  std::cout << "load balance worker start at port:" << k_yanghui_work_port4
            << std::endl;

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
  actor_status_monitor.RegisterActor(
      actor_for_load_balance_demo, "calculator for load balance",
      "a actor can calculate for load balance yanghui.");

  std::cout << "yanghui server ready to work, press 'q' to stop." << std::endl;
  actor_status_service.Run();
  actor_system::cluster::Cluster::GetInstance()->NotifyReady();

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

caf::behavior result_print_actor(caf::event_based_actor* self) {
  return {[](int result) {
    std::cout << "load balance count yanghui complete, get result:" << result
              << std::endl;
  }};
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
  //  actor_union_count_cluster counter(cfg.root_host, system,
  //  cfg.node_keeper_port,
  //                                 cfg.worker_port);

  //  balance_count_cluster balance_count_cluster(cfg.root_host, system);

  CountCluster* count_cluster;

  CDCF_LOGGER_INFO("Actor system log, hello world, I'm root.");

  auto* actor_cluster = new ActorUnionCountCluster(
      cfg.root_host, system, cfg.node_keeper_port, cfg.worker_port);
  count_cluster = actor_cluster;

  count_cluster->InitWorkerNodes();

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

  auto yanghui_load_balance_result = system.spawn(result_print_actor);
  auto yanghui_load_balance_get_min =
      system.spawn(yanghui_get_final_result, actor_cluster->load_balance_,
                   yanghui_load_balance_result);

  auto yanghui_load_balance_count_path =
      system.spawn(yanghui_count_path, actor_cluster->load_balance_,
                   yanghui_load_balance_get_min);

  actor_status_service.Run();
  actor_system::cluster::Cluster::GetInstance()->NotifyReady();

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
      caf::anon_send(yanghui_load_balance_count_path, kYanghuiData2);

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
  cdcf::Logger::Init(cfg);
  CDCF_LOGGER_DEBUG("I am {}", cfg.role_);
  if (cfg.role_ == "root") {
    SmartRootStart(system, cfg);
  } else {
    SmartWorkerStart(system, cfg);
  }
}

CAF_MAIN(caf::io::middleman)
