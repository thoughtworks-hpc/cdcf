//
// Created by Mingfei Deng on 2020/7/6.
//

#ifndef CDCF_BALANCECOUNTCLUSTER_H
#define CDCF_BALANCECOUNTCLUSTER_H

#include <iostream>
#include <actor_system.h>
#include "./CountCluster.h"

class BalanceCountCluster : public CountCluster{
 public:
  caf::actor_system& system_;
  caf::scoped_execution_unit context_;
  std::string host_;
  caf::actor counter_;
  caf::scoped_actor sender_actor_;
  BalanceCountCluster(std::string  host, caf::actor_system& system);
  ~BalanceCountCluster() override = default;

  void AddWorkerNodeWithPort(const std::string& host, uint16_t port);
  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;
};

#endif  // CDCF_BALANCECOUNTCLUSTER_H
