/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

class RouterPool : public caf::event_based_actor {
 public:
  RouterPool(caf::actor_config& cfg, std::string& name,
             std::string& routee_name, caf::message& routee_args,
             std::set<std::string>& routee_mpi, size_t& default_actor_num,
             caf::actor_pool::policy& policy);
  virtual ~RouterPool();
  void enqueue(caf::mailbox_element_ptr, caf::execution_unit*) override;

 private:
  void Down();
  void Exit();
  bool AddNode(const std::string& host, uint16_t port);
  bool DeleteNode(const std::string& host, uint16_t port);
  bool ModifyMaxPerNode(size_t size, const std::string& host, uint16_t port);
  bool DeleteActor(const std::string& key);
  bool AddActor(const caf::actor& gateway, const std::string& key);
  static std::string BuildWorkerKey(const std::string& host, uint16_t port);
  caf::actor GetSpawnActor(const std::string& host, uint16_t port);
  void DealOnExit(const caf::actor& actor, const std::string& key);

  std::string name_;
  size_t default_actor_num_;
  std::string factory_name_;
  caf::message factory_args_;
  std::set<std::string> mpi_;
  caf::actor pool_;
  // mutx
  std::mutex actor_lock_;
  // name(host, port) -- actors
  std::unordered_map<std::string, std::unordered_set<caf::actor>> nodes_;
};

}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
