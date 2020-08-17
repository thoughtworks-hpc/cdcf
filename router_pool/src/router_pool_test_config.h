/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_SRC_ROUTER_POOL_TEST_CONFIG_H_
#define ROUTER_POOL_SRC_ROUTER_POOL_TEST_CONFIG_H_

#include <cdcf/actor_system.h>

using simple_calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<caf::message>>;

simple_calculator::behavior_type simple_calculator_fun(
    simple_calculator::pointer self) {
  self->set_default_handler(caf::reflect_and_quit);
  return {[=](int a, int b) {
    if (a == -1 && b == -1) {
      self->quit();
      CDCF_LOGGER_ERROR(
          "actor_id:#{} received quit msg from remote node. input a:{} b:{}",
          self->id(), a, b);
      return caf::make_message(caf::make_error(caf::sec::bad_function_call));

    } else {
      CDCF_LOGGER_INFO(
          "actor_id:#{} received add_atom task from remote node. input a:{} "
          "b:{}",
          self->id(), a, b);
      return caf::make_message(a + b, self->id());
    }
  }};
}

class router_config : public actor_system::Config {
 public:
  router_config() {
    load<caf::io::middleman>();
    add_actor_type("simple_calculator", simple_calculator_fun);
  }
};

#endif  //  ROUTER_POOL_SRC_ROUTER_POOL_TEST_CONFIG_H_
