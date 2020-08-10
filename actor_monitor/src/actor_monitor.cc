/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/actor_monitor.h"

#include <logger.h>

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

        CDCF_LOGGER_INFO(
            "monitor new actor, actor addr:{} actor description:{}",
            caf::to_string(actor_addr), description);
      },
      [=](demonitor_atom, const std::string& actor_addr) {
        std::string description;
        {
          std::lock_guard<std::mutex> locker(actor_map_lock);
          description = actor_map_[actor_addr];
          actor_map_.erase(actor_addr);
        }
        CDCF_LOGGER_INFO(
            "stop monitor actor, actor addr:{} actor description:{}",
            actor_addr, description);
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
  // caf::anon_send(supervisor, worker.address(), description);

  supervisor->enqueue(nullptr, caf::make_message_id(),
                      caf::make_message(worker.address(), description),
                      nullptr);
  auto pointer = caf::actor_cast<caf::event_based_actor*>(supervisor);
  pointer->monitor(worker);
  return true;
}

bool StopMonitor(caf::actor& supervisor, const caf::actor_addr& worker) {
  caf::anon_send(supervisor, ActorMonitor::demonitor_atom::value,
                 caf::to_string(worker));
  auto pointer = caf::actor_cast<caf::event_based_actor*>(supervisor);
  pointer->demonitor(worker);
  return true;
}
