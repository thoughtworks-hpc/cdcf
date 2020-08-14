//
// Created by Mingfei Deng on 2020/8/13.
//

#ifndef CDCF_PING_PONG_CONFIG_H
#define CDCF_PING_PONG_CONFIG_H
#include <actor_system.h>

struct PingPongConfig : public actor_system::Config {
  std::string pong_host = "127.0.0.1";
  uint16_t pong_port = 58888;

  PingPongConfig() {
    opt_group{custom_options_, "global"}
        .add(pong_host, "pong_host", "set pong host")
        .add(pong_port, "pong_port", "set pong port");
  }
};

#endif  // CDCF_PING_PONG_CONFIG_H
