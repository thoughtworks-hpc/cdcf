/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "cdcf/actor_monitor.h"
#include "./actor_monitor_config.h"

void downMsgHandle(const caf::down_msg& downMsg,
                   const std::string& actor_description) {
  std::cout << std::endl;
  std::cout << "down actor address:" << caf::to_string(downMsg.source)
            << std::endl;
  std::cout << "down actor description:" << actor_description << std::endl;
  std::cout << "down reason:" << caf::to_string(downMsg.reason) << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
}

void caf_main(caf::actor_system& system, const config& cfg) {
  auto supervisor = system.spawn<ActorMonitor>(downMsgHandle);

  auto expected_port = caf::io::publish(supervisor, cfg.port);
  if (!expected_port) {
    std::cerr << "*** publish failed: " << system.render(expected_port.error())
              << std::endl;
    return;
  }

  std::cout << "supervisor started at port:" << expected_port << std::endl
            << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  std::cout << "... cya" << std::endl;
}

CAF_MAIN(caf::io::middleman)
