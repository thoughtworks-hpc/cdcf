/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./router_pool_worker.h"

#include <unordered_map>
#include <utility>

#include "./router_pool_message.h"
#include "caf/default_attachable.hpp"

namespace cdcf::router_pool {

RouterPoolWorker::RouterPoolWorker(caf::actor_config& cfg,
                                   std::shared_ptr<RouterPoolWorkerParam> param)
    : event_based_actor(cfg) {
  param_ = std::move(param);
  if (param_ != nullptr) {
    for (int i = 0; i < param_->GetMinNum(); i++) {
      CreateActor();
    }
  }
}

const char* RouterPoolWorker::name() const { return param_->GetName().c_str(); }

caf::behavior RouterPoolWorker::make_behavior() {
  caf::scoped_actor self(param_->GetSystem());
  RouterPoolMsgData ret;
  ret.min_num = param_->GetMinNum();
  ret.max_num = param_->GetMaxNum();
  return {[&](caf::get_atom) -> RouterPoolMsgData {
            // get actor
            std::lock_guard<std::mutex> mutx(mutex_);
            for (const auto& it : actor_set_) {
              ret.actors.push_back(it);
            }
            return ret;
          },
          [&](caf::add_atom, size_t num) -> RouterPoolMsgData {
            for (auto i = 0; i < num; i++) {
              auto actor = CreateActor();
              if (actor) {
                ret.actors.push_back(actor);
              }
            }
            return ret;
          },
          [&](const caf::down_msg&) { Exit(); }};
}

caf::actor RouterPoolWorker::CreateActor() {
  if (actor_set_.size() >= param_->GetMaxNum()) {
    return nullptr;
  }
  auto actor = param_->GetSpawnFactory()();
  if (actor != nullptr) {
    param_->GetActorMonitor().RegisterActor(actor, param_->GetName(),
                                            param_->GetDescription());
    actor->attach_functor([&](const caf::error& reason) {
      std::lock_guard<std::mutex> mutx(mutex_);
      auto iter = actor_set_.find(actor);
      if (iter != actor_set_.end()) {
        actor_set_.erase(iter);
      }
    });
    std::lock_guard<std::mutex> mutx(mutex_);
    actor_set_.insert(actor);
  }
  return actor;
}

void RouterPoolWorker::Exit() {
  std::lock_guard<std::mutex> mutx(mutex_);
  for (const auto& it : actor_set_) {
    anon_send_exit(it, caf::exit_reason::kill);
  }
}

std::shared_ptr<RouterPoolWorkerParam> RouterPoolWorkerParam::Make(
    caf::actor_system& system, ActorStatusMonitor& actor_monitor,
    const std::function<caf::actor()>& local_factory, std::string name,
    std::string description, size_t min_num, size_t max_num) {
  if (min_num > max_num || max_num == 0) {
    return nullptr;
  }
  caf::actor actor = local_factory();
  if (actor.node() != system.node()) {
    anon_send_exit(actor, caf::exit_reason::normal);
    return nullptr;
  }
  anon_send_exit(actor, caf::exit_reason::normal);
  std::shared_ptr<RouterPoolWorkerParam> ptr =
      std::shared_ptr<RouterPoolWorkerParam>(new RouterPoolWorkerParam(
          system, actor_monitor, local_factory, std::move(name),
          std::move(description), min_num, max_num));
  return ptr;
}

RouterPoolWorkerParam::RouterPoolWorkerParam(
    caf::actor_system& system, ActorStatusMonitor& actor_monitor,
    std::function<caf::actor()> local_factory, std::string name,
    std::string description, size_t min_num, size_t max_num)
    : system_(system), actor_monitor_(actor_monitor) {
  name_ = std::move(name);
  description_ = std::move(description);
  spawn_factory_ = std::move(local_factory);
  min_num_ = min_num;
  max_num_ = max_num;
}

const std::string& RouterPoolWorkerParam::GetName() { return name_; }

const std::string& RouterPoolWorkerParam::GetDescription() {
  return description_;
}

caf::actor_system& RouterPoolWorkerParam::GetSystem() { return system_; }

ActorStatusMonitor& RouterPoolWorkerParam::GetActorMonitor() {
  return actor_monitor_;
}

std::function<caf::actor()> RouterPoolWorkerParam::GetSpawnFactory() {
  return spawn_factory_;
}

size_t RouterPoolWorkerParam::GetMinNum() const { return min_num_; }

size_t RouterPoolWorkerParam::GetMaxNum() const { return max_num_; }

}  // namespace cdcf::router_pool
