/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_ROUTER_POOL_TEST_CONFIG_H
#define CDCF_ROUTER_POOL_TEST_CONFIG_H

#include <actor_system.h>

using simple_calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<int>>;
// simple_calculator::behavior_type
// simple_calculator_fun(simple_calculator::pointer self);

simple_calculator::behavior_type simple_calculator_fun(
    simple_calculator::pointer self) {
  self->set_default_handler(caf::reflect_and_quit);

  return {[=](int a, int b) {
    CDCF_LOGGER_INFO("received add_atom task from remote node. input a:{} b:{}",
                     a, b);
    return a + b;
  }};
}

class router_config : public actor_system::Config {
 public:
  uint16_t worker_port = 50099;

  router_config() {
    load<caf::io::middleman>();
    add_actor_type("simple_calculator", simple_calculator_fun);
  }
};

#endif  // CDCF_ROUTER_POOL_TEST_CONFIG_H
