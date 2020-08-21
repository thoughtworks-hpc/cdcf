/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf/actor_system.h>
#include <cdcf/logger.h>

#include <iostream>

#include "./include/ping_pong_config.h"

caf::behavior ping_fun(caf::event_based_actor* self,
                       const caf::actor& pong_actor) {
  self->send(pong_actor, 0);
  return {
      [=](int pong_value) -> int {
        CDCF_LOGGER_INFO("Get pong value:{}", pong_value);
        return ++pong_value;
      },
  };
}

class Ping : public cdcf::cluster::Observer {
 public:
  void Update(const cdcf::cluster::Event& event) override {
    if (event.member.name == "Pong") {
      auto remote_pong = system_.middleman().remote_actor(config_.pong_host,
                                                          config_.pong_port);
      if (nullptr == remote_pong) {
        CDCF_LOGGER_ERROR("get remote pong error");
      }

      system_.spawn(ping_fun, *remote_pong);
      CDCF_LOGGER_INFO("Start ping actor");
    }
  }
  Ping(caf::actor_system& system, const PingPongConfig& config)
      : system_(system), config_(config) {}

 private:
  caf::actor_system& system_;
  const PingPongConfig& config_;
};

void caf_main(caf::actor_system& system, const PingPongConfig& cfg) {
  Ping* ping = new Ping(system, cfg);
  cdcf::cluster::Cluster::GetInstance()->AddObserver(ping);
  cdcf::cluster::Cluster::GetInstance()->NotifyReady();
}

CAF_MAIN(caf::io::middleman)
