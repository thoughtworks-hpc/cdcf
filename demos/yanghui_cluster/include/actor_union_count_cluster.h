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

class ActorUnionCountCluster : public CountCluster {
 public:
  explicit ActorUnionCountCluster(std::string host, caf::actor_system& system,
                                  uint16_t port, uint16_t worker_port)
      : CountCluster(host),
        host_(std::move(host)),
        system_(system),
        port_(port),
        worker_port_(worker_port),
        counter_(system, caf::actor_pool::round_robin()) {}

  ~ActorUnionCountCluster() override = default;

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port);
  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

  caf::actor_system& system_;
  std::string host_;
  uint16_t port_;
  uint16_t worker_port_;
  ActorUnion counter_;
};

#endif  //  DEMOS_YANGHUI_CLUSTER_INCLUDE_ACTOR_UNION_COUNT_CLUSTER_H_
