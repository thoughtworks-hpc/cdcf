/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_H_

#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

using pool_atom = caf::atom_constant<caf::atom("pool")>;
using node_add_atom = caf::atom_constant<caf::atom("add_node")>;
using node_remove_atom = caf::atom_constant<caf::atom("del_node")>;
using modify_size_atom = caf::atom_constant<caf::atom("mdf_size")>;

class RouterPool : public caf::event_based_actor {
 public:
  RouterPool(caf::actor_config& cfg, std::string& name,
             std::string& description, std::string& factory_name,
             caf::message& factory_msg, std::set<std::string>& mpi,
             size_t& size);

  void enqueue(caf::mailbox_element_ptr, caf::execution_unit*) override;

 private:
  void Down();
  void Exit();
  void AddNode(const std::string& host, uint16_t port);
  void DeleteNode(const std::string& host, uint16_t port);
  void ModifySize(size_t size);
  void DeleteActor();
  void AddActor();
  caf::actor GetRemotePool(const std::string& host, uint16_t port,
                           const std::string& name);
  void GetPoolInfo(const std::string& host, uint16_t port, caf::actor& actor);
  static std::string BuildWorkerKey(const std::string& host, uint16_t port);

  std::string name_;
  std::string description_;
  size_t size_;
  std::string factory_name_;
  caf::message factory_args_;
  std::set<std::string> mpi_;
  caf::actor pool_;
  // mutx
  std::mutex actor_lock_;
  // local actors
  std::unordered_set<caf::actor> local_actors_;
  // name -- node
  std::unordered_map<std::string, caf::actor> nodes_;
  // node -- actor
  std::unordered_map<caf::actor, std::unordered_set<caf::actor>> remote_actors_;
};

}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
