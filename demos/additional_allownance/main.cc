/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf/cdcf_config.h>
#include <math.h>

#include <iostream>
#include <string>

#include "caf/scoped_actor.hpp"

using caf::actor;
using caf::actor_system;
using caf::behavior;
using caf::blocking_actor;
using caf::error;
using caf::event_based_actor;
using caf::scoped_actor;

behavior Additon(event_based_actor*) {
  return {[=](const int& what) -> int {
    int sum_value_1 = 0;
    for (int i = 1; i <= what; i++) sum_value_1 += i;

    return sum_value_1;
  }};
}

void Sum(blocking_actor* self) {
  auto handle_err = [&](const error& err) {
    aout(self) << "AUT (actor under test) failed: "
               << self->system().render(err) << std::endl;
  };
  bool running = true;

  self->receive_while(running)(
      [=](const actor& buddy, const int& what) -> int {
        int sum_value = 0;
        self->request(buddy, caf::infinite, what)
            .receive([=, &sum_value](const int& a) { sum_value = a; },
                     handle_err);
        return sum_value;
      },
      [&](caf::exit_msg& em) {
        if (em.reason) {
          self->fail_state(std::move(em.reason));
          running = false;
        }
      });
}

int main(int argc, char** argv) {
  CDCFConfig cfg;
  cfg.parse_config(argc, argv);
  caf::actor_system system{cfg};

  auto addtion_actor = system.spawn(Additon);
  auto sum_actor = system.spawn(Sum);
  scoped_actor self{system};
  caf::scheduler::abstract_coordinator& sch = system.scheduler();
  std::cout << "workers num:" << sch.num_workers() << std::endl;

  auto handle_err = [&](const error& err) {
    caf::aout(self) << "AUT (actor under test) failed: "
                    << self->system().render(err) << std::endl;
  };

  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  for (int i = 1; i < 5; i++) {
    int sum_value = 0;
    self->request(sum_actor, std::chrono::seconds(10), addtion_actor, i)
        .receive([=, &sum_value](const int& a) { sum_value = a; }, handle_err);

    std::cout << "sigma(1-" << i << ")=" << sum_value << std::endl;
  }
  std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

  auto time_used =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  std::cout << "time used:" << time_used.count() << "(S)" << std::endl;

  self->send_exit(sum_actor, caf::exit_reason::user_shutdown);
  return 0;
}
