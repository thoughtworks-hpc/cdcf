/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./router_pool.h"

#include <utility>

#include "caf/intrusive/lifo_inbox.hpp"

namespace cdcf::router_pool {

caf::actor RouterPoolMaster::Make(caf::execution_unit* eu, std::string name,
                                  std::string description, size_t min_num,
                                  size_t max_num) {
  auto& sys = eu->system();
  caf::actor_config cfg{eu};
  auto res = caf::make_actor<RouterPoolMaster, caf::actor>(
      sys.next_actor_id(), sys.node(), &sys, cfg);
  auto ptr = dynamic_cast<RouterPoolMaster*>(
      caf::actor_cast<caf::abstract_actor*>(res));
  ptr->eu_ = eu;
  ptr->name_ = std::move(name);
  ptr->description_ = std::move(description);
  ptr->min_num_ = min_num;
  ptr->max_num_ = max_num;
  ptr->max_num_per_work_ = 0;
  return res;
}

RouterPoolMaster::RouterPoolMaster(caf::actor_config& cfg)
    : monitorable_actor(cfg) {
  register_at_system();
  eu_ = nullptr;
  name_ = "";
  description_ = "";
  min_num_ = 0;
  max_num_ = 0;
  max_num_per_work_ = 0;
}

void RouterPoolMaster::on_destroy() {
  CAF_PUSH_AID_FROM_PTR(this);
  if (!getf(is_cleaned_up_flag)) {
    cleanup(caf::exit_reason::unreachable, nullptr);
    monitorable_actor::on_destroy();
    unregister_from_system();
  }
}

RouterPoolMaster::RouterPoolMaster(caf::actor_config& cfg,
                                   caf::execution_unit* eu, std::string name,
                                   std::string description, size_t min_num,
                                   size_t max_num)
    : caf::monitorable_actor(cfg) {
  eu_ = eu;
  name_ = std::move(name);
  description_ = std::move(description);
  min_num_ = min_num;
  max_num_ = max_num;
  max_num_per_work_ = 0;
  register_at_system();
}

void RouterPoolMaster::enqueue(caf::mailbox_element_ptr what,
                               caf::execution_unit* eu) {
  if (what->mid.is_response()) {
    ResponseMessage(std::move(what), eu);
  } else {
    PushMessage(what);
  }
  DispatcherMessage();
}

void RouterPoolMaster::DispatcherMessage() {
  caf::mailbox_element_ptr msg = PopMessage();
  if (msg == nullptr) {
    return;
  }
  Lock guard{actor_lock_};
  // find idler actor
  for (const auto& it : actors_) {
    if (!it.second->GetState()) {
      it.second->Proxy(ctrl(), msg, eu_);
      return;
    }
  }
  guard.unlock();
  // whether add an actor
  auto [new_actor, data] = AddActor();
  if (data != nullptr) {
    data->Proxy(ctrl(), msg, eu_);
  }
}

void RouterPoolMaster::PushMessage(caf::mailbox_element_ptr& what) {
  Lock guard{msg_lock_};
  messages_.push(std::move(what));
}

caf::mailbox_element_ptr RouterPoolMaster::PopMessage() {
  Lock guard{msg_lock_};
  if (messages_.empty()) {
    return nullptr;
  }
  caf::mailbox_element_ptr& front = messages_.front();
  messages_.pop();
  return std::move(front);
}

void RouterPoolMaster::ResponseMessage(caf::mailbox_element_ptr what,
                                       caf::execution_unit* eu) {
  auto from = caf::actor_cast<caf::actor>(what->sender);
  auto it = actors_.find(from);
  if (it != actors_.end()) {
    auto node = it->second;
    node->ResponseMessage(ctrl(), what, eu);
  }
  // whether reduce actor
  DeleteActor();
}

void RouterPoolMaster::AddWork(const std::string& host, uint16_t port) {
  auto node = std::shared_ptr<RouterPoolMasterWorker>(
      new RouterPoolMasterWorker(eu_->system(), host, port));
  node->Init();
  std::string key = BuildWorkerKey(host, port);
  Lock guard(actor_lock_);
  workers_.insert(std::make_pair(key, node));
  auto actors_map = node->GetActors();
  for (const auto& it : actors_map) {
    std::shared_ptr<RouterPoolActorData> data = nullptr;
    data = std::shared_ptr<RouterPoolActorData>(
        new RouterPoolActorData(it.first, node));
    auto item = std::pair<caf::actor, std::shared_ptr<RouterPoolActorData>>(
        it.first, data);
    actors_.insert(item);
  }
}

void RouterPoolMaster::DeleteWork(const std::string& host, uint16_t port) {
  std::string key = BuildWorkerKey(host, port);
  auto it = workers_.find(key);
  if (it == workers_.end()) {
    return;
  }
  std::shared_ptr<RouterPoolMasterWorker> worker = it->second;
  Lock guard(actor_lock_);
  workers_.erase(it);
  const std::unordered_map<caf::actor, bool>& remove_actors =
      worker->GetActors();
  for (const auto& delete_item : remove_actors) {
    auto iter = actors_.find(delete_item.first);
    if (iter != actors_.end()) {
      actors_.erase(iter);
      anon_send_exit(delete_item.first, caf::exit_reason::normal);
    }
  }
}

std::string RouterPoolMaster::BuildWorkerKey(const std::string& host,
                                             uint16_t port) {
  return host + std::to_string(port);
}

std::pair<caf::actor, std::shared_ptr<RouterPoolActorData>>
RouterPoolMaster::AddActor() {
  caf::actor actor = nullptr;
  std::shared_ptr<RouterPoolActorData> data = nullptr;
  for (const auto& it : workers_) {
    actor = it.second->AddActor();
    if (actor != nullptr) {
      data = std::shared_ptr<RouterPoolActorData>(
          new RouterPoolActorData(actor, it.second));
    }
  }
  auto item =
      std::pair<caf::actor, std::shared_ptr<RouterPoolActorData>>(actor, data);
  Lock guard(actor_lock_);
  actors_.insert(item);
  return item;
}

void RouterPoolMaster::DeleteActor() {
  Lock guard(actor_lock_);
  for (const auto& it : actors_) {
    if (!it.second->GetState()) {
      auto delete_actor = it.first;
      if (it.second->GetNode()->DeleteActor(delete_actor)) {
        actors_.erase(delete_actor);
        anon_send_exit(delete_actor, caf::exit_reason::normal);
        return;
      }
    }
  }
}

}  // namespace cdcf::router_pool
