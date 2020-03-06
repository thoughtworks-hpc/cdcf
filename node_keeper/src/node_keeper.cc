/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/node_keeper.h"

#include <memory>

namespace node_keeper {
NodeKeeper::NodeKeeper(const std::string& name, const gossip::Address& address,
                       const std::vector<gossip::Address>& seeds)
    : membership_() {
  std::shared_ptr<gossip::Transportable> transport =
      gossip::CreateTransport(address, address);

  membership::Config config;
  config.AddHostMember(name, address.host, address.port);

  const bool is_primary_seed = seeds.empty() || seeds[0] == address;
  if (!is_primary_seed) {
    for (auto& seed : seeds) {
      if (address != seed) {
        config.AddOneSeedMember("", seed.host, seed.port);
      }
    }
  }

  membership_.Init(transport, config);
}
}  // namespace node_keeper
