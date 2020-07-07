/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./router_pool_data.h"

#include <utility>

#include "./router_pool_message.h"

namespace cdcf::router_pool {
RouterPoolMasterWorker::RouterPoolMasterWorker(caf::actor_system& system,
                                               const std::string& host,
                                               uint16_t port)
    : system_(system) {
  host_ = host;
  port_ = port;
  gateway_ = nullptr;
  min_num_ = 0;
  max_num_ = 0;
}

bool RouterPoolMasterWorker::Init() {
  auto get_actor = system_.middleman().remote_actor(host_, port_);
  if (!get_actor) {
    std::cout << "connect remote actor failed. host:" << host_
              << ", port:" << port_ << std::endl;
    return false;
  }
  gateway_ = *get_actor;
  caf::scoped_actor self{system_};
  std::promise<RouterPoolMsgData> promise;
  self->request(gateway_, std::chrono::seconds(1), caf::get_atom::value)
      .receive(
          [&](const RouterPoolMsgData& ret) {
            promise.set_value(ret);
            std::cout << "=======add pool member host:" << host_
                      << ", port:" << port_ << std::endl;
          },
          [&](const caf::error& err) {
            std::cout << "====== add work node Error. host : " << host_
                      << ", port:" << port_ << std::endl;
            promise.set_exception(std::current_exception());
          });
  try {
    RouterPoolMsgData actors = promise.get_future().get();
    min_num_ = actors.min_num;
    max_num_ = actors.max_num;
    Lock guard{lock_};
    for (const auto& work_actor : actors.actors) {
      actors_.insert(std::make_pair(work_actor, false));
    }
  } catch (std::exception& e) {
    std::cout << "[exception caught: " << e.what() << "]" << std::endl;
    return false;
  }
  return true;
}

bool RouterPoolMasterWorker::DeleteActor(const caf::actor& actor) {
  Lock guard{lock_};
  if (actors_.size() <= min_num_) {
    return false;
  }
  auto it = actors_.find(actor);
  if (it != actors_.end()) {
    actors_.erase(it);
  }
  return true;
}

void RouterPoolMasterWorker::SetActorState(const caf::actor& actor,
                                           bool state) {
  auto it = actors_.find(actor);
  if (it != actors_.end()) {
    it->second = state;
  }
}

const std::unordered_map<caf::actor, bool>&
RouterPoolMasterWorker::GetActors() {
  return actors_;
}

caf::actor RouterPoolMasterWorker::AddActor() {
  if (gateway_ == nullptr) {
    return nullptr;
  }
  if (actors_.size() >= max_num_) {
    return nullptr;
  }
  caf::scoped_actor self{system_};
  std::promise<RouterPoolMsgData> promise;
  self->request(gateway_, std::chrono::seconds(1), caf::add_atom::value, 1)
      .receive(
          [&](const RouterPoolMsgData& ret) {
            promise.set_value(ret);
            std::cout << "=======add pool member host:" << host_
                      << ", port:" << port_ << std::endl;
          },
          [&](const caf::error& err) {
            std::cout << "====== add work node Error. host : " << host_
                      << ", port:" << port_ << std::endl;
            promise.set_exception(std::current_exception());
          });
  caf::actor work_actor = nullptr;
  try {
    RouterPoolMsgData actors = promise.get_future().get();
    if (actors.actors.size() == 1) {
      work_actor = actors.actors[0];
    }
    Lock guard{lock_};
    actors_.insert(std::make_pair(work_actor, false));
  } catch (std::exception& e) {
    std::cout << "[exception caught: " << e.what() << "]" << std::endl;
    return work_actor;
  }
  return work_actor;
}

RouterPoolActorData::RouterPoolActorData(
    const caf::actor& actor, std::shared_ptr<RouterPoolMasterWorker> node)
    : actor_(actor) {
  node_ = std::move(node);
  running_ = false;
  response_to_ = nullptr;
}

bool RouterPoolActorData::GetState() { return running_; }

std::shared_ptr<RouterPoolMasterWorker> RouterPoolActorData::GetNode() {
  return node_;
}

void RouterPoolActorData::ResponseMessage(const caf::strong_actor_ptr& ptr,
                                          caf::mailbox_element_ptr& what,
                                          caf::execution_unit* eu) {
  if (response_to_ != nullptr) {
    response_to_->enqueue(ptr, response_id_, what->move_content_to_message(),
                          eu);
    running_ = false;
    node_->SetActorState(actor_, running_);
  }
}

void RouterPoolActorData::Proxy(const caf::strong_actor_ptr& sender,
                                caf::mailbox_element_ptr& what,
                                caf::execution_unit* host) {
  response_id_ = what->mid.response_id();
  response_to_ = caf::actor_cast<caf::actor>(what->sender);
  new_id_ = AllocateRequestID(what);
  running_ = true;
  node_->SetActorState(actor_, running_);
  actor_->enqueue(sender, new_id_, what->move_content_to_message(), host);
}

caf::message_id RouterPoolActorData::AllocateRequestID(
    caf::mailbox_element_ptr& what) {
  auto priority = static_cast<caf::message_priority>(what->mid.category());
  caf::message_id last_request_id = caf::make_message_id();
  auto result = ++last_request_id;
  return priority == caf::message_priority::normal
             ? result
             : result.with_high_priority();
}
}  // namespace cdcf::router_pool
