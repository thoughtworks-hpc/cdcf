/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/count_cluster.h"

#include <utility>

#include "../../logger/include/logger.h"
CountCluster::CountCluster(std::string host) : host_(std::move(host)) {
  // auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
  // InitWorkerNodes(members, host_);
  actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
}

CountCluster::~CountCluster() {
  actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
}

void CountCluster::InitWorkerNodes() {
  auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();

  std::cout << "self, host: " << host_ << std::endl;
  std::cout << "members size:" << members.size() << std::endl;
  for (auto& m : members) {
    std::cout << "member, host: " << m.host << std::endl;
    if (m.hostname == host_ || m.host == host_) {
      continue;
    }
    std::cout << "add worker, host: " << m.host << std::endl;
    AddWorkerNode(m.host);
  }
}

void PrintClusterMembers() {
  auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
  std::cout << "Current Cluster Members:" << std::endl;
  for (int i = 0; i < members.size(); ++i) {
    auto& member = members[i];
    std::cout << "Member " << i + 1 << ": ";
    std::cout << "name: " << member.name << ", hostname: " << member.hostname
              << ", host: " << member.host << ", port:" << member.port
              << std::endl;
  }
}

void CountCluster::Update(const actor_system::cluster::Event& event) {
  CDCF_LOGGER_INFO("=======get update event, host: {}", event.member.host);

  if (event.member.hostname != host_ || event.member.host != host_) {
    if (event.member.status == event.member.ActorSystemUp) {
      //         std::this_thread::sleep_for(std::chrono::seconds(2));
      AddWorkerNode(event.member.host);
      PrintClusterMembers();
    } else if (event.member.status == event.member.Down) {
      CDCF_LOGGER_INFO("detect worker node down, host: {}, port:{}",
                       event.member.host, event.member.port);
    } else if (event.member.status == event.member.ActorSystemDown) {
      CDCF_LOGGER_INFO("detect worker actor system down, host: {}, port: {}",
                       event.member.host, event.member.port);
    }
  }
}
