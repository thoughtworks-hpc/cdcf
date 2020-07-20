/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./router_pool.h"

#include <unordered_set>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf::router_pool {

RouterPool::RouterPool(caf::actor_config& cfg, std::string& name,
                       std::string& description, std::string& factory_name,
                       caf::message& factory_msg, std::set<std::string>& mpi,
                       size_t& default_actor_num,
                       caf::actor_pool::policy& policy)
    : event_based_actor(cfg) {
  caf::actor_system& system_1 = system();
  caf::scoped_actor self{system_1};
  caf::scoped_execution_unit context{&system_1};
  pool_ = caf::actor_pool::make(&context, std::move(policy));
  name_ = name;
  description_ = description;
  factory_name_ = factory_name;
  factory_args_ = std::move(factory_msg);
  default_actor_num_ = default_actor_num;
  mpi_ = std::move(mpi);
}

void RouterPool::enqueue(caf::mailbox_element_ptr what,
                         caf::execution_unit* host) {
  const auto& content = what->content();
  if (content.match_elements<caf::exit_msg>()) {
    send(pool_, caf::exit_msg());
    caf::event_based_actor::enqueue(std::move(what), host);
  } else if (content.match_elements<caf::down_msg>()) {
    Down(const_cast<caf::down_msg&>(content.get_as<caf::down_msg>(0)));
  } else if (content.match_elements<caf::sys_atom, caf::add_atom, node_atom,
                                    std::string, uint16_t>()) {
    // sys add node host port
    bool ret =
        AddNode(content.get_as<std::string>(3), content.get_as<uint16_t>(4));
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content.match_elements<caf::sys_atom, caf::delete_atom, node_atom,
                                    std::string, uint16_t>()) {
    // sys delete node host port
    auto ret =
        DeleteNode(content.get_as<std::string>(3), content.get_as<uint16_t>(4));
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content
                 .match_elements<caf::sys_atom, caf::get_atom, node_atom>()) {
    // sys get node
    auto ret = GetNode();
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content
                 .match_elements<caf::sys_atom, caf::get_atom, actor_atom>()) {
    // sys get actor
    auto ret = GetActors();
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content.match_elements<caf::sys_atom, caf::get_atom, actor_atom,
                                    std::string, uint16_t>()) {
    // sys get actor host port
    auto ret =
        GetActors(content.get_as<std::string>(3), content.get_as<uint16_t>(4));
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content
                 .match_elements<caf::sys_atom, caf::update_atom, size_t>()) {
    // sys update size
    auto ret = ModifyMaxPerNode(content.get_as<size_t>(2));
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else if (content.match_elements<caf::sys_atom, caf::update_atom, size_t,
                                    std::string, uint16_t>()) {
    // sys update size port host
    auto ret = ModifyMaxPerNode(content.get_as<size_t>(2),
                                content.get_as<std::string>(3),
                                content.get_as<uint16_t>(4));
    what->sender->enqueue(nullptr, what->mid.response_id(),
                          caf::make_message(ret), host);
  } else {
    pool_->enqueue(std::move(what), host);
  }
}

std::string RouterPool::BuildNodeKey(const std::string& host, uint16_t port) {
  if (host.empty()) {
    return "";
  }
  return host + ":" + std::to_string(port);
}

std::tuple<std::string, uint16_t> RouterPool::ParserNodeKey(
    const std::string& key) {
  if (key.empty()) {
    return std::move(std::tuple<std::string, uint16_t>("", 0));
  }
  size_t pos = key.find_last_of(':');
  std::string host = key.substr(0, pos);
  uint16_t port = (uint16_t)std::stoi(key.substr(pos + 1));
  return std::tuple<std::string, uint16_t>(host, port);
}

void RouterPool::Down(caf::down_msg& msg) {
  DeleteActor(msg.source);
  send(pool_, msg);
}

bool RouterPool::AddNode(const std::string& host, uint16_t port) {
  auto gateway = GetSpawnActor(host, port);
  if (!host.empty()) {
    if (gateway == nullptr) {
      return false;
    }
  }
  std::string key = BuildNodeKey(host, port);
  if (nodes_.find(key) != nodes_.end()) {
    return false;
  }
  std::unique_lock<std::mutex> ul(actor_lock_);
  nodes_.insert(
      std::make_pair(key, std::move(std::unordered_set<caf::actor>())));
  ul.unlock();
  for (int i = 0; i < default_actor_num_; i++) {
    if (!AddActor(gateway, key)) {
      return false;
    }
  }
  return true;
}

bool RouterPool::DeleteNode(const std::string& host, uint16_t port) {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  std::string key = BuildNodeKey(host, port);
  auto node_it = nodes_.find(key);
  if (node_it == nodes_.end()) {
    return false;
  }
  auto actors = std::move(node_it->second);
  nodes_.erase(node_it);
  for (const auto& it : actors) {
    anon_send(it, caf::exit_reason::user_shutdown);
    // anon_send(pool_, caf::sys_atom(), caf::delete_atom(), it);
  }
  return true;
}

std::vector<std::string> RouterPool::GetNode() {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  std::vector<std::string> result;
  for (auto& it : nodes_) {
    result.push_back(it.first);
  }
  return std::move(result);
}

bool RouterPool::ModifyMaxPerNode(size_t size) {
  default_actor_num_ = size;
  for (auto& node_it : nodes_) {
    auto [host, port] = ParserNodeKey(node_it.first);
    ModifyMaxPerNode(size, host, port);
  }
  return true;
}

bool RouterPool::ModifyMaxPerNode(size_t size, const std::string& host,
                                  uint16_t port) {
  std::string key = BuildNodeKey(host, port);
  auto it = nodes_.find(key);
  if (it == nodes_.end()) {
    return false;
  }
  auto pre_size = it->second.size();
  if (pre_size > size) {
    return DeleteActor(key, pre_size - size);
  }
  if (pre_size < size) {
    caf::actor gateway = nullptr;
    if (!key.empty()) {
      gateway = GetSpawnActor(host, port);
      if (gateway == nullptr) {
        return false;
      }
    }
    for (int i = 0; i < size - pre_size; i++) {
      if (!AddActor(gateway, key)) {
        return false;
      }
    }
  }
  return true;
}

bool RouterPool::DeleteActor(caf::actor_addr& actor_addr) {
  //  aout(this) << "delete actor : " << actor_addr << "" << std::endl;
  std::unique_lock<std::mutex> mutx(actor_lock_);
  for (auto& node_it : nodes_) {
    for (auto& actor_it : node_it.second) {
      if (actor_it.address() == actor_addr) {
        this->demonitor(actor_it);
        node_it.second.erase(actor_it);
        return true;
      }
    }
  }
  return false;
}

bool RouterPool::DeleteActor(const std::string& key, size_t num) {
  std::lock_guard<std::mutex> mutx(actor_lock_);
  auto it = nodes_.find(key);
  if (it == nodes_.end()) {
    return false;
  }
  auto& actor_set = it->second;
  size_t count = 0;
  for (auto& actor : actor_set) {
    if (count >= num) {
      break;
    }
    anon_send(actor, caf::exit_reason::user_shutdown);
    count++;
  }
  return true;
}

bool RouterPool::AddActor(const caf::actor& gateway, const std::string& key) {
  std::unique_lock<std::mutex> mutx(actor_lock_);
  auto it = nodes_.find(key);
  if (it == nodes_.end()) {
    return false;
  }
  std::unordered_set<caf::actor>& actor_set = it->second;
  caf::actor add_actor = nullptr;
  if (gateway == nullptr && key.empty()) {
    // local spawn
    auto res = system().spawn<caf::actor>(factory_name_, factory_args_, nullptr,
                                          true, &mpi_);
    if (res) {
      add_actor = std::move(*res);
    }
  } else {
    // remote spawn
    std::promise<caf::actor> promise;
    auto tout = std::chrono::seconds(30);  // wait no longer than 30s
    caf::scoped_actor self(system());
    self->request(gateway, tout, caf::spawn_atom::value, factory_name_,
                  factory_args_, mpi_, name_, description_)
        .receive([&](caf::actor& ret) { promise.set_value(std::move(ret)); },
                 [&](caf::error& err) {
                   std::cerr << "can't spawn actor from gateway" << std::endl;
                   promise.set_value(nullptr);
                 });
    add_actor = promise.get_future().get();
  }
  if (add_actor != nullptr) {
    anon_send(pool_, caf::sys_atom::value, caf::put_atom::value, add_actor);
    actor_set.insert(add_actor);
    this->monitor(add_actor);
    return true;
  }
  return false;
}

caf::actor RouterPool::GetSpawnActor(const std::string& host, uint16_t port) {
  if (host.empty()) {
    return nullptr;
  }
  auto gateway = system().middleman().remote_actor(host, port);
  if (!gateway) {
    std::cerr << "*** connect failed: " << to_string(gateway.error())
              << std::endl;
    return nullptr;
  }
  return *gateway;
}

std::vector<caf::actor> RouterPool::GetActors() {
  std::unique_lock<std::mutex> ul(actor_lock_);
  std::vector<caf::actor> result;
  for (auto& node_it : nodes_) {
    for (auto& actor_it : node_it.second) {
      result.push_back(actor_it);
    }
  }
  return std::move(result);
}

std::vector<caf::actor> RouterPool::GetActors(const std::string& host,
                                              uint16_t port) {
  std::unique_lock<std::mutex> ul(actor_lock_);
  std::string key = BuildNodeKey(host, port);
  auto node_it = nodes_.find(key);
  if (node_it == nodes_.end()) {
    return std::vector<caf::actor>();
  }
  auto& actor_set = node_it->second;
  std::vector<caf::actor> result;
  result.reserve(actor_set.size());
  for (auto& actor : actor_set) {
    result.push_back(actor);
  }
  return std::move(result);
}

RouterPool::~RouterPool() {
  caf::scoped_actor self{system()};
  self->send(pool_, caf::infinite, caf::exit_reason::kill);
  self->wait_for();
}

}  // namespace cdcf::router_pool
