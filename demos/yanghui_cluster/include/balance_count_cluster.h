/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_BALANCE_COUNT_CLUSTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_BALANCE_COUNT_CLUSTER_H_

#include <actor_system.h>

#include <iostream>
#include <string>
#include <vector>

#include "./count_cluster.h"

class balance_count_cluster : public count_cluster {
 public:
  caf::actor_system& system_;
  caf::scoped_execution_unit context_;
  std::string host_;
  caf::actor counter_;
  caf::scoped_actor sender_actor_;
  balance_count_cluster(std::string host, caf::actor_system& system);
  ~balance_count_cluster() override = default;

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port);
  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_BALANCE_COUNT_CLUSTER_H_
