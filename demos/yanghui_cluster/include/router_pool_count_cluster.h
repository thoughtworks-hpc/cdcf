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
                         const std::string& node_keeper_host,
                         uint16_t node_keeper_port, uint16_t worker_port,
                         std::string& routee_name, caf::message& routee_args,
                         caf::actor_system::mpi& routee_ifs,
                         size_t& default_actor_num,
                         caf::actor_pool::policy& policy, bool use_ssl);
  virtual ~RouterPoolCountCluster();

  void Update(const actor_system::cluster::Event& event) override;

  void AddWorkerNode(const std::string& host) override;
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

  std::string NodeList();
  size_t GetPoolSize();
  size_t GetPoolSize(const std::string& host, uint16_t port);
  bool ChangePoolSize(size_t size);
  bool ChangePoolSize(size_t size, const std::string& host, uint16_t port);
  bool RemoveNode(const std::string& host, uint16_t port);
  bool AddNode(const std::string& host, uint16_t port);

 private:
  caf::actor_system& system_;
  std::string host_;
  uint16_t worker_port_;
  caf::actor pool_;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_ROUTER_POOL_COUNT_CLUSTER_H_
