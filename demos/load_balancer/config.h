/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_CONFIG_H_
#define DEMOS_LOAD_BALANCER_CONFIG_H_
#include <actor_system.h>

#include "./worker.h"

class Config : public actor_system::Config {
 public:
  uint16_t reception_port_{3335};

 public:
  Config() {
    add_actor_type("Calculator", CalculatorFun);
    opt_group{custom_options_, "global"}.add(
        reception_port_, "reception_port,r", "port for reception");
  }
};
#endif  // DEMOS_LOAD_BALANCER_CONFIG_H_
