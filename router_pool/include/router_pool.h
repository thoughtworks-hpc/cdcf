/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_H_

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

#include "./router_pool_data.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

using Lock = caf::upgrade_lock<caf::detail::shared_spinlock>;

class RouterPoolMaster : public caf::monitorable_actor {
 public:
  static caf::actor Make(caf::execution_unit* eu, std::string name,
                         std::string description, size_t min_num,
                         size_t max_num);
  explicit RouterPoolMaster(caf::actor_config& cfg);

  RouterPoolMaster(caf::actor_config& cfg, caf::execution_unit* eu,
                   std::string name, std::string description, size_t min_num,
                   size_t max_num);
  void on_destroy() override;
  void AddWork(const std::string& host, uint16_t port);
  void DeleteWork(const std::string& host, uint16_t port);

  void enqueue(caf::mailbox_element_ptr, caf::execution_unit*) override;

 private:
  std::pair<caf::actor, std::shared_ptr<RouterPoolActorData>> AddActor();
  void DeleteActor();

  void PushMessage(caf::mailbox_element_ptr&);
  void DispatcherMessage();
  void ResponseMessage(caf::mailbox_element_ptr, caf::execution_unit*);
  caf::mailbox_element_ptr PopMessage();

  static std::string BuildWorkerKey(const std::string& host, uint16_t port);

  std::string name_;
  std::string description_;
  caf::execution_unit* eu_;
  size_t min_num_;
  size_t max_num_;
  size_t max_num_per_work_;
  // message
  caf::detail::shared_spinlock msg_lock_;
  std::queue<caf::mailbox_element_ptr> messages_;
  // worker
  caf::detail::shared_spinlock actor_lock_;
  std::unordered_map<std::string, std::shared_ptr<RouterPoolMasterWorker>>
      workers_;
  std::unordered_map<caf::actor, std::shared_ptr<RouterPoolActorData>> actors_;
};

}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_H_
