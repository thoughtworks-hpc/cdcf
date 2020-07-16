/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_ROUTER_POOL_COUNT_CLUSTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_ROUTER_POOL_COUNT_CLUSTER_H_

#include <string>
#include <vector>

#include "../../../router_pool/include/router_pool.h"
#include "./count_cluster.h"

class RouterPoolCountCluster : public CountCluster {
 public:
  RouterPoolCountCluster(std::string host, caf::actor_system& system,
                         uint16_t port, uint16_t worker_port,
                         std::string& routee_name, caf::message& routee_args,
                         caf::actor_system::mpi& routee_ifs,
                         size_t& default_actor_num,
                         caf::actor_pool::policy& policy);

  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

 private:
  caf::actor_system& system_;
  std::string host_;
  uint16_t port_;
  uint16_t worker_port_;
  caf::actor pool_;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_ROUTER_POOL_COUNT_CLUSTER_H_
