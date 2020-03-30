/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_HELLO_WORLD_WORKER_H_
#define DEMOS_HELLO_WORLD_WORKER_H_
#include <actor_system.h>

#include <string>

extern std::string the_host_name;

using Hello = caf::typed_actor<caf::replies_to<std::string>::with<std::string>>;

Hello::behavior_type HelloFun(Hello::pointer self) {
  return {
      [=](std::string s) -> std::string {
        caf::aout(self) << "received task from a remote node" << std::endl;
        return s + " from " + the_host_name + "\n";
      },
  };
}
#endif  // DEMOS_HELLO_WORLD_WORKER_H_
