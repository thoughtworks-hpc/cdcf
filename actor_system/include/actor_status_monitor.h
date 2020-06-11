//
// Created by Mingfei Deng on 2020/6/5.
//

#ifndef CDCF_ACTOR_STATUS_MONITOR_H
#define CDCF_ACTOR_STATUS_MONITOR_H

#include <map>
#include <utility>
#include <vector>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../actor_monitor/include/actor_monitor.h"

class ActorStatusMonitor {
 public:
  struct ActorInfo {
    std::uint64_t id;
    std::string name;
    std::string description;
  };

  explicit ActorStatusMonitor(caf::actor_system& actorSystem);
  void RegisterActor(caf::actor& actor, const std::string& name,
                     const std::string& description = "");
  std::vector<ActorInfo> GetActorStatus();

 private:
  caf::actor_system& actor_system_;
  std::unordered_map<caf::actor_id, ActorInfo> actor_status_;
  std::mutex actor_status_lock_;
  caf::actor actor_monitor_;
};

#endif  // CDCF_ACTOR_STATUS_MONITOR_H
