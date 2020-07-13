/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/count_cluster.h"

#include <utility>
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

void CountCluster::Update(const actor_system::cluster::Event& event) {
  std::cout << "=======get update event, host:" << event.member.host
            << std::endl;

  if (event.member.hostname != host_) {
    if (event.member.status == event.member.Up) {
      // std::this_thread::sleep_for(std::chrono::seconds(2));
      AddWorkerNode(event.member.host);
    } else {
      // Todo(Yujia.Li): resource leak
      std::cout << "detect worker node down, host:" << event.member.host
                << " port:" << event.member.port << std::endl;
    }
  }
}
