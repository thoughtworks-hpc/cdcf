/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/node_keeper.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

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

void NodeKeeper::Run() {
  using membership::Member;
  std::vector<Member> older_members;

  auto comparator = [](const Member& lhs, const Member& rhs) {
    return lhs.GetNodeName() < rhs.GetNodeName();
  };
  auto interval = std::chrono::seconds(1);
  for (;; std::this_thread::sleep_for(interval)) {
    auto newer_members = membership_.GetMembers();

    std::vector<Member> up_members;
    std::set_difference(newer_members.begin(), newer_members.end(),
                        older_members.begin(), older_members.end(),
                        std::back_inserter(up_members), comparator);
    for (auto& member : up_members) {
      std::cout << "node [" << member.GetNodeName() << "] is up." << std::endl;
    }

    std::vector<Member> down_members;
    std::set_difference(older_members.begin(), older_members.end(),
                        newer_members.begin(), newer_members.end(),
                        std::back_inserter(down_members), comparator);
    for (auto& member : down_members) {
      std::cout << "node [" << member.GetNodeName() << "] is down."
                << std::endl;
    }

    older_members = std::move(newer_members);
  }
}
}  // namespace node_keeper
