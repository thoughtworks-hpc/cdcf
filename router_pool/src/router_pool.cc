/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./router_pool.h"

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

RouterPool::RouterPool(caf::actor_config& cfg, std::string& pool_name,
                       std::string& pool_description, std::string& factory_name,
                       caf::message& factory_msg, std::set<std::string>& mpi,
                       size_t& size)
    : event_based_actor(cfg) {
  caf::actor_system& system_1 = system();
  caf::scoped_actor self{system_1};
  caf::scoped_execution_unit context{&system_1};
  pool_ = caf::actor_pool::make(&context, caf::actor_pool::round_robin());
  name_ = pool_name;
  description_ = pool_description;
  factory_name_ = factory_name;
  factory_args_ = std::move(factory_msg);
  size_ = size;
  mpi_ = std::move(mpi);
}

void RouterPool::enqueue(caf::mailbox_element_ptr what,
                         caf::execution_unit* host) {
  const auto& content = what->content();
  if (content.match_elements<caf::exit_msg>()) {
    Exit();
  } else if (content.match_elements<caf::down_msg>()) {
    Down();
  } else if (content.match_elements<pool_atom, node_add_atom, std::string,
                                    uint16_t>()) {
    AddNode(content.get_as<std::string>(2), content.get_as<uint16_t>(3));
  } else if (content.match_elements<pool_atom, node_remove_atom, std::string,
                                    uint16_t>()) {
    DeleteNode(content.get_as<std::string>(2), content.get_as<uint16_t>(3));
  } else if (content.match_elements<pool_atom, modify_size_atom, size_t>()) {
    ModifySize(content.get_as<size_t>(2));
  } else {
    pool_->enqueue(std::move(what), host);
  }
}

std::string RouterPool::BuildWorkerKey(const std::string& host, uint16_t port) {
  return host + std::to_string(port);
}

void RouterPool::Down() { send(pool_, caf::down_msg()); }

void RouterPool::Exit() { send(pool_, caf::exit_msg()); }

void RouterPool::AddNode(const std::string& host, uint16_t port) {
  std::string key = BuildWorkerKey(host, port);
  if (nodes_.find(key) != nodes_.end()) {
    DeleteNode(host, port);
  }
  std::lock_guard<std::mutex> mutx(actor_lock_);
  std::string pool_name = "";  // TODO(fengkai): impl
  // 判断远端是否已经存在 同名 pool
  //   如果不存在
  //   将构建pool的信息发送至远端构建
  // 获取远端的pool
  auto remote_pool = GetRemotePool(host, port, pool_name);
  // 获取远端pool里的actor信息，并将actor信息存储在本节点中
  GetPoolInfo(host, port, remote_pool);
}

void RouterPool::DeleteNode(const std::string& host, uint16_t port) {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  std::string key = BuildWorkerKey(host, port);
  auto node_it = nodes_.find(key);
  if (node_it == nodes_.end()) {
    return;
  }
  auto remote_pool = node_it->second;
  nodes_.erase(node_it);
  auto pool_it = remote_actors_.find(remote_pool);
  if (pool_it == remote_actors_.end()) {
    return;
  }
  std::unordered_set<caf::actor> actors = pool_it->second;
  remote_actors_.erase(pool_it);
  for (const auto& it : actors) {
    anon_send(pool_, caf::sys_atom(), caf::delete_atom(), it);
  }
}

void RouterPool::ModifySize(size_t size) {
  caf::scoped_actor self(system());
  std::promise<int> promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) { promise.set_value(ret.size()); },
          [&](caf::error& err) { promise.set_value(-1); });
  int ret = promise.get_future().get();
  if (ret > 0 && size != ret) {
    if (size > ret) {
      for (int i = 0; i < size - ret; i++) {
        DeleteActor();
      }
    } else {
      for (int i = 0; i < ret - size; i++) {
        AddActor();
      }
    }
  }
}

void RouterPool::DeleteActor() {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  if (local_actors_.size() == 0) {
    return;
  }
  auto del_it = local_actors_.begin();
  send(pool_, caf::sys_atom(), caf::delete_atom(), *del_it);
  anon_send(*del_it, caf::exit_reason::user_shutdown);
  local_actors_.erase(del_it);
}

void RouterPool::AddActor() {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  auto res = system().spawn<caf::actor>(factory_name_, factory_args_, nullptr,
                                        true, &mpi_);
  if (res) {
    local_actors_.insert(*res);
    send(pool_, caf::sys_atom(), caf::put_atom(), *res);
  }
}

caf::actor RouterPool::GetRemotePool(const std::string& host, uint16_t port,
                                     const std::string& name) {
  // TODO(fengkai): impl
  return caf::actor();
}

void RouterPool::GetPoolInfo(const std::string& host, uint16_t port,
                             caf::actor& pool) {
  std::string key = BuildWorkerKey(host, port);
  caf::scoped_actor self(system());
  std::promise<std::vector<caf::actor>> promise;
  self->request(pool, caf::infinite, caf::sys_atom::value, caf::get_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) {
            promise.set_value(std::move(ret));
          },
          [&](caf::error& err) {
            promise.set_exception(std::current_exception());
          });
  try {
    std::vector<caf::actor> actors = promise.get_future().get();
    std::unordered_set<caf::actor> actor_set;
    for (const auto& actor : actors) {
      actor_set.insert(std::move(actor));
    }
    nodes_.insert(std::make_pair(key, pool));
    remote_actors_.insert(std::make_pair(pool, std::move(actor_set)));
  } catch (std::exception& e) {
    std::cout << "[exception caught: " << e.what() << "]" << std::endl;
  }
}

}  // namespace cdcf::router_pool
