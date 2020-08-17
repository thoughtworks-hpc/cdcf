/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf/actor_system.h>
#include <logger.h>

#include "./include/ping_pong_config.h"

// this struct will use to add cluster
class Pong : public actor_system::cluster::Observer {
 public:
  void Update(const actor_system::cluster::Event& event) override {}

  Pong() {}
};

caf::behavior pong_fun(caf::event_based_actor* self) {
  return {
      [=](int ping_value) -> int {
        CDCF_LOGGER_INFO("Get ping_value:", ping_value);
        return ++ping_value;
      },
  };
}

// the main function of application
void caf_main(caf::actor_system& system, const PingPongConfig& cfg) {
  Pong* pong = new Pong();
  system.spawn(pong_fun);

  // add node to cluster
  actor_system::cluster::Cluster::GetInstance()->AddObserver(pong);

  // start pong actor
  auto pong_actor = system.spawn(pong_fun);

  // publish the pong actor, so remote node can send message to it
  system.middleman().publish(pong_actor, cfg.pong_port);
  CDCF_LOGGER_INFO("pong_actor start at port:{}", cfg.pong_port);

  // notify cluster the application is start read
  actor_system::cluster::Cluster::GetInstance()->NotifyReady();
}

// this code is necessary
CAF_MAIN(caf::io::middleman)
