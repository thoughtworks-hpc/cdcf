/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf/actor_system.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "./config.h"
#include "./server.h"

void caf_main(caf::actor_system& system, const Config& config) {
  std::cout << "this node host: " << config.host_ << std::endl;
  std::cout << "this node name: " << config.name_ << std::endl;

  auto actor_system_port = config.port_;
  if (config.name_ == "server") {
    try {
      asio::io_context io_context;
      Server s(io_context, system, config.host_, actor_system_port,
               config.reception_port_);
      io_context.run();
    } catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << "\n";
    }
  } else {
    InitWorker(system, actor_system_port);
    while (true) {
      std::cin.get();
    }
  }
}

CAF_MAIN(caf::io::middleman)
