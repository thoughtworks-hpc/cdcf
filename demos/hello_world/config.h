/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_HELLO_WORLD_CONFIG_H_
#define DEMOS_HELLO_WORLD_CONFIG_H_
#include <cdcf/actor_system.h>

#include "./worker.h"

class Config : public cdcf::actor_system::Config {
 public:
  uint16_t reception_port_ = 3335;

 public:
  Config() {
    add_actor_type("Hello", HelloFun);
    opt_group{custom_options_, "global"}.add(
        reception_port_, "reception_port,r", "port for reception");
  }
};
#endif  // DEMOS_HELLO_WORLD_CONFIG_H_
