/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_WORKER_H_
#define DEMOS_LOAD_BALANCER_WORKER_H_
#include <actor_system.h>

caf::behavior CalculatorFun(caf::stateful_actor<size_t>* self) {
  return {
      [=](int a, int b) -> int {
        caf::aout(self) << "received task from a remote node" << std::endl;
        ++self->state;
        return a + b;
      },
      [=](int dummy) { return self->state; },
  };
}

void InitWorker(caf::actor_system& system, uint16_t port) {
  std::cout << "actor system port: " << port << std::endl;
  auto res = system.middleman().open(port);
  if (!res) {
    std::cerr << "*** cannot open port: " << system.render(res.error())
              << std::endl;
    return;
  }
}
#endif  // DEMOS_LOAD_BALANCER_WORKER_H_
