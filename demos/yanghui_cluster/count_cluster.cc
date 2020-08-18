/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/count_cluster.h"

#include <utility>

#include "cdcf/logger.h"
// CountCluster::CountCluster(std::string host) : host_(std::move(host)) {
//  // auto members =
//  cdcf::cluster::Cluster::GetInstance()->GetMembers();
//  // InitWorkerNodes(members, host_);
//  cdcf::cluster::Cluster::GetInstance()->AddObserver(this);
//}

CountCluster::CountCluster(std::string host,
                           const std::string& node_keeper_host,
                           uint16_t node_keeper_port)
    : host_(std::move(host)) {
  if (node_keeper_host.empty()) {
    cdcf::cluster::Cluster::GetInstance()->AddObserver(this);
  } else {
    cdcf::cluster::Cluster::GetInstance(node_keeper_host,
                                                node_keeper_port)
        ->AddObserver(this);
  }
}

CountCluster::~CountCluster() {
  cdcf::cluster::Cluster::GetInstance()->RemoveObserver(this);
}

const char k_role_worker[] = "worker";

void CountCluster::InitWorkerNodes() {
  auto members = cdcf::cluster::Cluster::GetInstance()->GetMembers();

  std::cout << "self, host: " << host_ << std::endl;
  std::cout << "members size:" << members.size() << std::endl;
  for (auto& m : members) {
    std::cout << "member, host: " << m.host << std::endl;
    std::cout << "role: " << m.role << std::endl;
    if (m.role == k_role_worker) {
      std::cout << "add worker, host: " << m.hostname << std::endl;
      AddWorkerNode(m.host);
    }
  }
}

void PrintClusterMembers() {
  auto members = cdcf::cluster::Cluster::GetInstance()->GetMembers();
  std::cout << "Current Cluster Members:" << std::endl;
  for (int i = 0; i < members.size(); ++i) {
    auto& member = members[i];
    std::cout << "Member " << i + 1 << ": ";
    std::cout << "name: " << member.name << ", hostname: " << member.hostname
              << ", host: " << member.host << ", port:" << member.port
              << std::endl;
  }
}

void CountCluster::Update(const cdcf::cluster::Event& event) {
  if (event.member.hostname != host_ || event.member.host != host_) {
    if (event.member.status == event.member.ActorSystemUp) {
      CDCF_LOGGER_DEBUG("Actor system up, host: {}, role: {}",
                        event.member.host, event.member.role);
      //         std::this_thread::sleep_for(std::chrono::seconds(2));
      if (event.member.role == k_role_worker) {
        AddWorkerNode(event.member.host);
      }
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
