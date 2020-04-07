/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/config.h"
#include "src/node_keeper.h"

int main(int argc, char *argv[]) {
  node_keeper::Config config;
  auto ret = config.parse_config(argc, argv, "cdcf-default.ini");
  if (ret != CDCFConfig::RetValue::kSuccess) {
    return 1;
  }

  auto seeds = config.GetSeeds();
  if (seeds.size()) {
    std::cout << "seeding with: " << std::endl;
    for (size_t i = 0; i < seeds.size(); ++i) {
      std::cout << "\t" << seeds[i].host << ":" << seeds[i].port << std::endl;
    }
  }
  node_keeper::NodeKeeper keeper(config.name_, {config.host_, config.port_},
                                 seeds);
  keeper.Run();
  return 0;
}
