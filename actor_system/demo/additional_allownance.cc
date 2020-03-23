/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf_config.h>
#include <math.h>

#include <iostream>
#include <string>

#include "caf/scoped_actor.hpp"

using namespace caf;

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
        self->request(buddy, infinite, what)
            .receive([=, &sum_value](const int& a) { sum_value = a; },
                     handle_err);
        return sum_value;
      },
      [&](exit_msg& em) {
        if (em.reason) {
          self->fail_state(std::move(em.reason));
          running = false;
        }
      });
}

int ImplementSum(scoped_actor& self, const actor& a1, const actor& a2,
                 const int& number, const int& id) {
  auto handle_err = [&](const error& err) {
    aout(self) << "AUT (actor under test) failed: "
               << self->system().render(err) << std::endl;
  };

  int sum_value = 0;
  self->request(a1, std::chrono::seconds(10), a2, number)
      .receive([=, &sum_value](const int& a) { sum_value = a; }, handle_err);
  return sum_value;
}

int main(int argc, char** argv) {
  cdcf_config cfg;
  cfg.parse_config(argc, argv);
  actor_system system{cfg};

  auto addtion_actor = system.spawn(Additon);
  auto sum_actor = system.spawn(Sum);
  scoped_actor self{system};
  scheduler::abstract_coordinator& sch = system.scheduler();
  std::cout << "workers num:" << sch.num_workers() << std::endl;

  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  for (int i = 1; i < 5; i++) {
    int value = ImplementSum(self, sum_actor, addtion_actor, i, i);
    std::cout << "sigma(1-" << i << ")=" << value << std::endl;
  }
  std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

  auto time_used =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  std::cout << "time used:" << time_used.count() << "(S)" << std::endl;

  self->send_exit(sum_actor, exit_reason::user_shutdown);
  return 0;
}
