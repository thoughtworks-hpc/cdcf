/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <chrono>
#include <thread>

#include "./actor_monitor_config.h"
#include "cdcf/actor_monitor.h"

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

caf::behavior calculator_fun(caf::event_based_actor* self) {
  self->set_default_handler(caf::reflect_and_quit);
  return {[=](int a, int b) {
            std::cout << "worker quit" << std::endl;
            self->quit();
          },
          [](sub_atom, int a, int b) { return a - b; }};
}

void caf_main(caf::actor_system& system, const config& cfg) {
  // get remote monitor actor
  auto supervisorActor = system.middleman().remote_actor(cfg.host, cfg.port);
  auto supervisor = std::move(*supervisorActor);

  // build worker actor
  auto worker = system.spawn(calculator_fun);

  // set monitor
  cdcf::actor_system::SetMonitor(supervisor, worker,
                                 "worker actor for testing");

  std::cout << "worker running" << std::endl;

  auto worker_fun = make_function_view(worker);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  if (cfg.test_type == "down") {
    // quit in actor run
    std::cout << "actor quit when run." << std::endl;
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

  if (cfg.test_type == "default") {
    // send error
    std::cout << "actor receive not define message." << std::endl;
    self->send(worker, "unknown message");
  }

  std::cout << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);

  std::cout << "... cya" << std::endl;
  caf::anon_send_exit(worker, caf::exit_reason::user_shutdown);
}

CAF_MAIN(caf::io::middleman)
