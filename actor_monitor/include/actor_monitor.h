/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
#define ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
#include <map>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

class ActorMonitor : public caf::event_based_actor {
 public:
  explicit ActorMonitor(caf::actor_config& cfg);
  ActorMonitor(caf::actor_config& cfg,
               std::function<void(const caf::down_msg&, const std::string&)>&&
                   downMsgFun);
  caf::behavior make_behavior() override;

 private:
  std::mutex actor_map_lock;
  std::function<void(const caf::down_msg& down_msg,
                     const std::string& description)>
      down_msg_fun;
  std::map<std::string, std::string> actor_map_;
  std::map<std::string, caf::actor_addr> actor_addr_map_;
  void DownMsgHandle(const caf::down_msg& down_msg,
                     const std::string& description);
};

bool SetMonitor(caf::actor& supervisor, caf::actor& worker,
                const std::string& description);

#endif  // ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
