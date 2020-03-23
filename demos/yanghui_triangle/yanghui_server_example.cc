/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./yanghui_config.h"

void caf_main(caf::actor_system& system, const config& cfg) {
  auto res = system.middleman().publish_local_groups(cfg.port);
  if (!res) {
    std::cerr << "*** publishing local groups failed: "
              << system.render(res.error()) << std::endl;
    return;
  }
  std::cout << "*** listening at port " << *res << std::endl
            << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  std::cout << "... cya" << std::endl;
}

CAF_MAIN(caf::io::middleman)
