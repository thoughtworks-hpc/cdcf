/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_SIMPLE_PING_PONG_INCLUDE_PING_PONG_CONFIG_H_
#define DEMOS_SIMPLE_PING_PONG_INCLUDE_PING_PONG_CONFIG_H_
#include <actor_system.h>

#include <string>

struct PingPongConfig : public actor_system::Config {
  std::string pong_host = "127.0.0.1";
  uint16_t pong_port = 58888;

  PingPongConfig() {
    opt_group{custom_options_, "global"}
        .add(pong_host, "pong_host", "set pong host")
        .add(pong_port, "pong_port", "set pong port");
  }
};

#endif  // DEMOS_SIMPLE_PING_PONG_INCLUDE_PING_PONG_CONFIG_H_
