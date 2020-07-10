/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/node_keeper.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "src/event.h"
#include "src/grpc.h"
#include "src/node_status_grpc_impl.h"

namespace node_keeper {
NodeKeeper::NodeKeeper(const Config& config) : membership_() {
  std::string name = config.name_;
  gossip::Address address{config.host_, config.port_};
  auto seeds = config.GetSeeds();

  std::shared_ptr<gossip::Transportable> transport =
      gossip::CreateTransport(address, address);

  membership::Config membership_config;
  membership_config.SetHostMember(name, address.host, address.port);
  membership_config.SetLoggerName(logger_name_);

  const bool is_primary_seed = seeds.empty() || seeds[0] == address;
  if (!is_primary_seed) {
    for (auto& seed : seeds) {
      if (address != seed) {
        membership_config.AddOneSeedMember("", seed.host, seed.port);
      }
    }
  }

  membership_.Init(transport, membership_config);

  transport->Run();
}

class Subscriber : public membership::Subscriber {
 public:
  void Update() override {
    { std::lock_guard lock(mutex_); }
    condition_variable_.notify_all();
  }

  void Wait() {
    std::unique_lock lock(mutex_);
    condition_variable_.wait(lock);
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_variable_;
};

[[noreturn]] void NodeKeeper::Run() {
  std::string server_address("0.0.0.0:50051");
  GRPCImpl service(membership_);
  NodeStatusGRPCImpl node_status_service(membership_);
  GRPCServer server(server_address, {&service, &node_status_service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  auto subscriber = std::make_shared<Subscriber>();
  membership_.Subscribe(subscriber);
  MemberEventGenerator generator;
  for (;;) {
    CDCF_LOGGER_DEBUG("generate event");
    auto events = generator.Update(membership_.GetMembers(),
                                   membership_.GetActorSystems());
    for (auto& event : events) {
      switch (event.type) {
        case MemberEvent::kMemberUp:
          std::cout << "node [" << event.member.GetNodeName() << "@"
                    << event.member.GetHostName() << ": "
                    << event.member.GetIpAddress() << ":"
                    << event.member.GetPort() << "] is up." << std::endl;
          service.Notify({{node_keeper::MemberEvent::kMemberUp, event.member}});
          break;
        case MemberEvent::kMemberDown:
          std::cout << "node [" << event.member.GetNodeName() << "@"
                    << event.member.GetHostName() << ": "
                    << event.member.GetIpAddress() << ":"
                    << event.member.GetPort() << "] is down." << std::endl;
          service.Notify(
              {{node_keeper::MemberEvent::kMemberDown, event.member}});
          break;
        case MemberEvent::kActorSystemDown:
          CDCF_LOGGER_DEBUG("Send actor system down event to self actor system.");
          service.Notify(
              {{node_keeper::MemberEvent::kActorSystemDown, event.member}});
          break;
        case MemberEvent::kActorSystemUp:
          CDCF_LOGGER_DEBUG("Send actor system up event to self actor system.");
          service.Notify(
              {{node_keeper::MemberEvent::kActorSystemUp, event.member}});
          break;
      }
    }
    subscriber->Wait();
  }
}
}  // namespace node_keeper
