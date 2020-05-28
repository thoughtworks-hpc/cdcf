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

namespace node_keeper {
NodeKeeper::NodeKeeper(const std::string& name, const gossip::Address& address,
                       const std::vector<gossip::Address>& seeds,
                       const std::string& logfile)
    : membership_() {
  std::shared_ptr<gossip::Transportable> transport =
      gossip::CreateTransport(address, address);

  membership::Config config;
  config.SetHostMember(name, address.host, address.port);
  if (!logfile.empty()) {
    config.SetLogFilePathAndName(logfile);
  }

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

void NodeKeeper::Run() {
  std::string server_address("0.0.0.0:50051");
  GRPCImpl service;
  GRPCServer server(server_address, {&service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  auto subscriber = std::make_shared<Subscriber>();
  membership_.Subscribe(subscriber);
  MemberEventGenerator generator;
  for (;;) {
    /* FIXME: Is GetMembers thread safe? */
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
    subscriber->Wait();
  }
}
}  // namespace node_keeper
