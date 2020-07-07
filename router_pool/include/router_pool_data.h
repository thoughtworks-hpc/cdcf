/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_DATA_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_DATA_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

class RouterPoolMasterWorker {
 public:
  using Lock = caf::upgrade_lock<caf::detail::shared_spinlock>;

  RouterPoolMasterWorker(caf::actor_system& system, const std::string& host,
                         uint16_t port);
  bool Init();
  bool DeleteActor(const caf::actor& actors);
  void SetActorState(const caf::actor& actor, bool state);
  const std::unordered_map<caf::actor, bool>& GetActors();
  caf::actor AddActor();

 private:
  caf::actor_system& system_;
  std::string host_;
  uint16_t port_;
  size_t min_num_;
  size_t max_num_;
  caf::actor gateway_;
  std::unordered_map<caf::actor, bool> actors_;
  caf::detail::shared_spinlock lock_;
};

class RouterPoolActorData {
 public:
  RouterPoolActorData(const caf::actor& actor,
                      std::shared_ptr<RouterPoolMasterWorker> node);
  bool GetState();
  std::shared_ptr<RouterPoolMasterWorker> GetNode();
  void Proxy(const caf::strong_actor_ptr& sender,
             caf::mailbox_element_ptr& what, caf::execution_unit* host);
  void ResponseMessage(const caf::strong_actor_ptr& ptr,
                       caf::mailbox_element_ptr& what, caf::execution_unit* eu);

 private:
  caf::message_id AllocateRequestID(caf::mailbox_element_ptr& what);

  caf::message_id new_id_;
  caf::message_id response_id_;
  caf::actor response_to_;
  std::atomic<bool> running_{};
  const caf::actor& actor_;
  std::shared_ptr<RouterPoolMasterWorker> node_;
};
}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_DATA_H_
