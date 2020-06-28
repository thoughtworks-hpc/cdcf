/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "../include/actor_status_monitor.h"

ActorStatusMonitor::ActorStatusMonitor(caf::actor_system& actorSystem,
                                       actor_system::cluster::Cluster* cluster)
    : actor_system_(actorSystem), cluster_(cluster) {
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
  if (cluster_) {
    cluster_->PushActorsUpToNodeKeeper({actor});
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
