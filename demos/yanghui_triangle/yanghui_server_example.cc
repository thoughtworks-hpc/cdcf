/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "yanghui_config.h"

using namespace std;

void caf_main(caf::actor_system& system, const config& cfg) {
  auto res = system.middleman().publish_local_groups(cfg.port);
  if (!res) {
    std::cerr << "*** publishing local groups failed: "
              << system.render(res.error()) << endl;
    return;
  }
  cout << "*** listening at port " << *res << endl
       << "*** press [enter] to quit" << endl;
  string dummy;
  std::getline(std::cin, dummy);
  cout << "... cya" << endl;
}

CAF_MAIN(caf::io::middleman)