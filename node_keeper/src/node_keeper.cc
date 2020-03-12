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

#include "src/event.h"
#include "src/grpc.h"

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
  std::string server_address("0.0.0.0:50051");
  GRPCImpl service;
  GRPCServer server(server_address, {&service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  MemberEventGenerator generator;
  auto interval = std::chrono::seconds(1);
  for (;; std::this_thread::sleep_for(interval)) {
    auto events = generator.Update(membership_.GetMembers());
    for (auto& event : events) {
      std::cout << "node [" << event.member.GetNodeName();
      switch (event.type) {
        case MemberEvent::kMemberUp:
          std::cout << "] is up." << std::endl;
          service.Notify({{node_keeper::MemberEvent::kMemberUp, event.member}});
          break;
        case MemberEvent::kMemberDown:
          std::cout << "] is down." << std::endl;
          service.Notify(
              {{node_keeper::MemberEvent::kMemberDown, event.member}});
          break;
      }
    }
  }
}
}  // namespace node_keeper
