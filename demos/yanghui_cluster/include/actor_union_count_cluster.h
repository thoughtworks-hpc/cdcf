/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_ACTOR_UNION_COUNT_CLUSTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_ACTOR_UNION_COUNT_CLUSTER_H_

#include <string>
#include <utility>
#include <vector>

#include "../../../actor_fault_tolerance/include/actor_union.h"
#include "./count_cluster.h"
#include "./yanghui_io.h"

class ActorUnionCountCluster : public CountCluster {
 public:
  explicit ActorUnionCountCluster(std::string host, caf::actor_system& system,
                                  const std::string& node_keeper_host,
                                  uint16_t node_keeper_port,
                                  uint16_t worker_port, YanghuiIO& yanghui_io)
      : CountCluster(host, node_keeper_host, node_keeper_port),

        host_(std::move(host)),
        system_(system),
        context_(&system_),
        worker_port_(worker_port),
        counter_(system, caf::actor_pool::round_robin()),
        yanghui_io_(yanghui_io) {
    auto policy = cdcf::load_balancer::policy::MinLoad(1);
    load_balance_ =
        cdcf::load_balancer::Router::Make(&context_, std::move(policy));
  }

  ~ActorUnionCountCluster() override = default;

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port);
  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

  caf::actor_system& system_;
  std::string host_;
  uint16_t worker_port_;
  ActorUnion counter_;
  caf::scoped_execution_unit context_;
  caf::actor load_balance_;
  YanghuiIO& yanghui_io_;
};

#endif  //  DEMOS_YANGHUI_CLUSTER_INCLUDE_ACTOR_UNION_COUNT_CLUSTER_H_
