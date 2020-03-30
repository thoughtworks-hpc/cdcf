//
// Created by Mingfei Deng on 2020/3/26.
//

#include "../include/actor_monitor.h"

ActorMonitor::ActorMonitor(caf::actor_config& cfg) : event_based_actor(cfg) {}
ActorMonitor::ActorMonitor(
    caf::actor_config& cfg,
    const std::function<void(const caf::down_msg&, const std::string&)>&
        downMsgFun)
    : event_based_actor(cfg), down_msg_fun(downMsgFun) {}

caf::behavior ActorMonitor::make_behavior() {
  set_down_handler([=](caf::down_msg msg) {
    mapLock.lock();
    std::string description = actor_map_[caf::to_string(msg.source)];
    mapLock.unlock();

    if (down_msg_fun != nullptr) {
      down_msg_fun(msg, description);
    } else {
      DownMsgHandle(msg, description);
    }
  });

  return {
      [=](std::string msg) { std::cout << msg << std::endl; },
      [=](const caf::actor_addr& actor_addr, const std::string description) {
        mapLock.lock();
        actor_map_[caf::to_string(actor_addr)] = description;
        mapLock.unlock();
        aout(this) << "monitor new actor, actor addr:"
                   << caf::to_string(actor_addr)
                   << "actor description:" << description << std::endl;
      }};
}

void ActorMonitor::AddMoMonitored(std::string actor_unity,
                                  std::string description) {
  mapLock.lock();
  actor_map_[actor_unity] = description;
  mapLock.unlock();
}

void ActorMonitor::DownMsgHandle(const caf::down_msg& down_msg,
                                 const std::string& description) {
  std::stringstream buffer;
  buffer << "actor:" << caf::to_string(down_msg.source)
         << " down, reason:" << caf::to_string(down_msg.reason)
         << ". actor description:" << description;

  this->send(this, buffer.str());
}

void ActorMonitor::SetDownMsgFun(
    const std::function<void(const caf::down_msg&, const std::string&)>&
        downMsgFun) {
  down_msg_fun = downMsgFun;
}

bool SetMonitor(caf::actor& supervisor, caf::actor& worker,
                const std::string& description) {
  caf::anon_send(supervisor, worker.address(), description);
  auto pointer = caf::actor_cast<caf::event_based_actor*>(supervisor);
  pointer->monitor(worker);
  return true;
}
