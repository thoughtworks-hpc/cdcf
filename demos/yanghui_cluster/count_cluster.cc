/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/count_cluster.h"

#include <utility>
count_cluster::count_cluster(std::string host) : host_(std::move(host)) {
  auto members = actor_system::cluster::Cluster::GetInstance()->GetMembers();
  InitWorkerNodes(members, host_);
  actor_system::cluster::Cluster::GetInstance()->AddObserver(this);
}

count_cluster::~count_cluster() {
  actor_system::cluster::Cluster::GetInstance()->RemoveObserver(this);
}

void count_cluster::InitWorkerNodes(
    const std::vector<actor_system::cluster::Member>& members,
    const std::string& host) {
  std::cout << "self, host: " << host << std::endl;
  std::cout << "members size:" << members.size() << std::endl;
  for (auto& m : members) {
    std::cout << "member, host: " << m.host << std::endl;
    if (m.host == host) {
      continue;
    }
    std::cout << "add worker, host: " << m.host << std::endl;
    AddWorkerNode(m.host);
  }
}

void count_cluster::Update(const actor_system::cluster::Event& event) {
  std::cout << "=======get update event, host:" << event.member.host
            << std::endl;

  if (event.member.host != host_) {
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
