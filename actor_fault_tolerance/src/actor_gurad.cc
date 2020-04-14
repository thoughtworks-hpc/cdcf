/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/actor_guard.h"

ActorGuard::ActorGuard(caf::actor& keepActor,
                       std::function<caf::actor(std::atomic<bool>&)> restart,
                       caf::actor_system& system)
    : message_guarantor_(keepActor, system), restart_fun_(std::move(restart)) {
  monitor_ = system.spawn<ActorMonitor>(
      [=](const caf::down_msg& msg, const std::string& description) {
        active_ = false;
        HandleDownMsg(caf::to_string(msg.source));
      });

  // SetMonitor(monitor_, keepActor, caf::to_string(keepActor));
}

bool ActorGuard::SendMsg(MessageID msg_id, const caf::message& msg) {
  message_guarantor_.SendMsg(msg_id, msg);
}

void ActorGuard::ConfirmMsg(MessageID msg_id) {
  message_guarantor_.ConfirmMsg(msg_id);
}

void ActorGuard::HandleDownMsg(const std::string& actor_description) {
  message_guarantor_.SetReceiver(restart_fun_(active_));
  std::cout << "actor:" << actor_description << "is down." << std::endl;

  if (active_) {
    std::cout << "actor:" << actor_description << "is restart";
    message_guarantor_.ReSendMsg();
  } else {
    std::cout << "actor:" << actor_description << "restart failed";
  }
}
