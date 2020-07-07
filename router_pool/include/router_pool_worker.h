/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_WORKER_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_WORKER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "./actor_status_monitor.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

class RouterPoolWorkerParam {
 public:
  static std::shared_ptr<RouterPoolWorkerParam> Make(
      caf::actor_system& system, ActorStatusMonitor& actor_monitor,
      const std::function<caf::actor()>& local_factory, std::string name,
      std::string description, size_t min_num, size_t max_num);

 private:
  RouterPoolWorkerParam(caf::actor_system& system,
                        ActorStatusMonitor& actor_monitor,
                        std::function<caf::actor()> local_factory,
                        std::string name, std::string description,
                        size_t min_num, size_t max_num);

  std::string name_;
  std::string description_;
  caf::actor_system& system_;
  ActorStatusMonitor& actor_monitor_;
  std::function<caf::actor()> spawn_factory_;
  size_t min_num_;
  size_t max_num_;

 public:
  const std::string& GetName();
  const std::string& GetDescription();
  caf::actor_system& GetSystem();
  ActorStatusMonitor& GetActorMonitor();
  std::function<caf::actor()> GetSpawnFactory();
  size_t GetMinNum() const;
  size_t GetMaxNum() const;
};

class RouterPoolWorker : public caf::event_based_actor {
 public:
  RouterPoolWorker(caf::actor_config& cfg,
                   std::shared_ptr<RouterPoolWorkerParam> param);

  caf::behavior make_behavior() override;
  const char* name() const override;

 private:
  caf::actor CreateActor();
  void Exit();

  std::shared_ptr<RouterPoolWorkerParam> param_;
  std::mutex mutex_;
  std::unordered_set<caf::actor> actor_set_;
};
}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_WORKER_H_
