/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "cdcf/actor_status_monitor.h"

ActorStatusMonitor::ActorStatusMonitor(caf::actor_system& actorSystem)
    : actor_system_(actorSystem) {
  actor_monitor_ = actor_system_.spawn<ActorMonitor>(
      [&](const caf::down_msg& msg, const std::string&) {
        std::lock_guard<std::mutex> lock_guard(actor_status_lock_);
        actor_status_.erase(msg.source.id());
      });
}

void ActorStatusMonitor::RegisterActor(caf::actor& actor,
                                       const std::string& name,
                                       const std::string& description) {
  {
    std::lock_guard<std::mutex> lock_guard(actor_status_lock_);
    actor_status_[actor.id()] = {actor.id(), name, description};
  }
  SetMonitor(actor_monitor_, actor, description);
}

std::vector<ActorStatusMonitor::ActorInfo>
ActorStatusMonitor::GetActorStatus() {
  std::vector<ActorStatusMonitor::ActorInfo> ret;
  std::lock_guard<std::mutex> lock_guard(actor_status_lock_);

  for (auto& actor_status : actor_status_) {
    ret.push_back(actor_status.second);
  }

  return ret;
}
ActorStatusMonitor::~ActorStatusMonitor() {
  // must send kill to actor_monitor_, otherwise the actor_monitor_ will not
  // stop
  caf::anon_send_exit(actor_monitor_, caf::exit_reason::user_shutdown);
}
