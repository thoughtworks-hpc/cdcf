/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <unistd.h>

#include "../../actor_monitor/include/actor_monitor.h"
#include "./actor_monitor_config.h"

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

// caf::behavior supervisor_fun(caf::event_based_actor* self,
//                             const caf::actor worker) {
//  // self->monitor(worker);
//  return {[=](caf::down_msg msg) { std::cout << "get down msg" << std::endl;
//  }};
//}

caf::behavior calculator_fun(caf::event_based_actor* self) {
  return {[=](int a, int b) {
            std::cout << "worker quit" << std::endl;
            self->quit();
            return;
          },
          [](sub_atom, int a, int b) { return a - b; }};
}

void caf_main(caf::actor_system& system, const config& cfg) {
  // get remote monitor actor
  auto supervisorActor = system.middleman().remote_actor(cfg.host, cfg.port);
  auto supervisor = std::move(*supervisorActor);

  // build worker actor
  auto worker = system.spawn(calculator_fun);
  // auto localSupervisor = system.spawn(supervisor_fun, worker);
  //  auto pointer = caf::actor_cast<caf::event_based_actor*>(supervisor);
  //  pointer->monitor(worker);

  // set monitor
  SetMonitor(supervisor, worker, "worker actor for testing");

  std::cout << "worker running" << std::endl;

  // sleep
  sleep(2);

  if (cfg.test_type == "down") {
    // quit in actor run
    std::cout << "actor quit when run." << std::endl;
    auto worker_fun = make_function_view(worker);
    worker_fun(12, 13);
  }

  caf::scoped_actor self{system};

  if (cfg.test_type == "exit") {
    // send exit
    std::cout << "actor receive system exit message." << std::endl;
    self->send_exit(worker, caf::exit_reason::kill);
  }

  if (cfg.test_type == "error") {
    // send error
    std::cout << "actor receive can't handle error." << std::endl;
    self->send(worker, caf::make_error(caf::exit_reason::out_of_workers));
  }

  std::cout << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);

  std::cout << "... cya" << std::endl;
  caf::anon_send_exit(worker, caf::exit_reason::user_shutdown);
}

CAF_MAIN(caf::io::middleman)