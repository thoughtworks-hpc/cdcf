/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_WORKER_H_
#define DEMOS_LOAD_BALANCER_WORKER_H_
#include <cdcf/actor_system.h>

caf::behavior CalculatorFun(caf::stateful_actor<size_t>* self) {
  return {
      [=](size_t x) {
        ++self->state;
        size_t result = 1;
        for (size_t i = 1; i <= x; ++i) {
          result *= i;
        }
        return result;
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
