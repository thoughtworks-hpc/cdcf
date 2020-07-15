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
#include "include/actor_union_count_cluster.h"
#include "include/cdcf_spawn.h"
#include "include/router_pool_count_cluster.h"
#include "include/yanghui_actor.h"
#include "include/yanghui_demo_calculator.h"

void SmartWorkerStart(caf::actor_system& system, const config& cfg) {
  ActorStatusMonitor actor_status_monitor(system);
  ActorStatusServiceGprcImpl actor_status_service(system, actor_status_monitor);

  auto cdcf_spawn = system.spawn<CdcfSpawn>(&actor_status_monitor);
  actor_status_service.Run();

  system.middleman().publish(cdcf_spawn, cfg.worker_port);
  std::cout << "yanghui server ready to work, press 'q' to stop." << std::endl;

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
  CountCluster* count_cluster;

  CDCF_LOGGER_INFO("Actor system log, hello world, I'm root.");
  std::string routee_name = "calculator";
  auto routee_args = caf::make_message();
  auto routee_ifs = system.message_types<calculator>();
  auto policy = caf::actor_pool::round_robin();
  size_t default_size = 3;
  count_cluster = new RouterPoolCountCluster(
      cfg.root_host, system, cfg.node_keeper_port, cfg.worker_port, routee_name,
      routee_args, routee_ifs, default_size, policy);

  count_cluster->InitWorkerNodes();

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
      actor_guard.SendAndReceive(printRet, dealSendErr, kYanghuiData2);

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
  if (cfg.root) {
    SmartRootStart(system, cfg);
  } else {
    SmartWorkerStart(system, cfg);
  }
}

CAF_MAIN(caf::io::middleman)
