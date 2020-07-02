/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/actor_monitor.h"

#include "../../actor_system/include/actor_system/cluster.h"

ActorMonitor::ActorMonitor(caf::actor_config& cfg) : event_based_actor(cfg) {}
ActorMonitor::ActorMonitor(
    caf::actor_config& cfg,
    std::function<void(const caf::down_msg&, const std::string&)>&& downMsgFun)
    : event_based_actor(cfg), down_msg_fun(std::move(downMsgFun)) {}

caf::behavior ActorMonitor::make_behavior() {
  set_down_handler([=](const caf::down_msg& msg) {
    std::string description;

    {
      std::lock_guard<std::mutex> locker(actor_map_lock);
      description = actor_map_[caf::to_string(msg.source)];
    }

    if (down_msg_fun != nullptr) {
      down_msg_fun(msg, description);
    } else {
      DownMsgHandle(msg, description);
    }
  });

  return {
      [=](const std::string& msg) { std::cout << msg << std::endl; },
      [=](const caf::actor_addr& actor_addr, const std::string& description) {
        {
          std::lock_guard<std::mutex> locker(actor_map_lock);
          actor_map_[caf::to_string(actor_addr)] = description;
        }
        actor_addr_map_[caf::to_string(actor_addr)] = actor_addr;

        aout(this) << "monitor new actor, actor addr:"
                   << caf::to_string(actor_addr)
                   << "actor description:" << description << std::endl;
      },
      [=](const std::vector<std::string>& down_actor_address) {
        // TODO: 这里好像保存地址有问题，而且不用cluster通知的话，monitor好像也能感知到
//        for (auto& address : down_actor_address) {
//          if (auto it = actor_addr_map_.find(address);
//              it != actor_addr_map_.end()) {
//            caf::down_msg msg;
//            msg.source = it->second;
//            std::cout << "xxx, address: " << address << ", real address:" << caf::to_string(it->second) << std::endl;
//            this->send(this, msg);
//          }
//        }
      }};
}

void ActorMonitor::DownMsgHandle(const caf::down_msg& down_msg,
                                 const std::string& description) {
  std::stringstream buffer;
  buffer << "actor:" << caf::to_string(down_msg.source)
         << " down, reason:" << caf::to_string(down_msg.reason)
         << ". actor description:" << description;

  this->send(this, buffer.str());
}

bool SetMonitor(caf::actor& supervisor, caf::actor& worker,
                const std::string& description) {
  caf::anon_send(supervisor, worker.address(), description);
  auto pointer = caf::actor_cast<caf::event_based_actor*>(supervisor);
  pointer->monitor(worker);
  return true;
}
