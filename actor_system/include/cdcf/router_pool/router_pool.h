/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_SYSTEM_INCLUDE_CDCF_ROUTER_POOL_ROUTER_POOL_H_
#define ACTOR_SYSTEM_INCLUDE_CDCF_ROUTER_POOL_ROUTER_POOL_H_

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

using node_atom = caf::atom_constant<caf::atom("node")>;
using actor_atom = caf::atom_constant<caf::atom("actor")>;

class RouterPool : public caf::event_based_actor {
 public:
  RouterPool(caf::actor_config& cfg, caf::actor_system& system,
             std::string& name, std::string& description,
             std::string& routee_name, caf::message& routee_args,
             std::set<std::string>& routee_mpi, size_t& default_actor_num,
             caf::actor_pool::policy& policy, bool use_ssl);
  ~RouterPool() override;
  void enqueue(caf::mailbox_element_ptr, caf::execution_unit*) override;

 private:
  void Down(caf::down_msg& nsg);
  bool AddNode(const std::string& host, uint16_t port);
  bool DeleteNode(const std::string& host, uint16_t port);
  std::vector<std::string> GetNode();
  bool ModifyMaxPerNode(size_t size);
  bool ModifyMaxPerNode(size_t size, const std::string& host, uint16_t port);
  bool DeleteActor(const std::string& key, size_t num);
  bool DeleteActor(caf::actor_addr& actor);
  bool AddActor(const caf::actor& gateway, const std::string& key);
  std::vector<caf::actor> GetActors();
  std::vector<caf::actor> GetActors(const std::string& host, uint16_t port);
  caf::actor GetSpawnActor(const std::string& host, uint16_t port);

  static std::string BuildNodeKey(const std::string& host, uint16_t port);
  static std::tuple<std::string, uint16_t> ParserNodeKey(
      const std::string& key);

  std::string name_;
  std::string description_;
  size_t default_actor_num_;
  std::string factory_name_;
  caf::message factory_args_;
  std::set<std::string> mpi_;
  caf::actor pool_;
  const bool use_ssl_;
  caf::actor_system& system_;
  // mutx
  std::mutex actor_lock_;
  // name(host:port) -- actors
  std::unordered_map<std::string, std::unordered_set<caf::actor>> nodes_;
};

}  // namespace cdcf::router_pool

#endif  // ACTOR_SYSTEM_INCLUDE_CDCF_ROUTER_POOL_ROUTER_POOL_H_
