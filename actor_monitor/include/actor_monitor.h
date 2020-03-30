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
               const std::function<void(const caf::down_msg&,
                                        const std::string&)>& downMsgFun);
  caf::behavior make_behavior() override;
  void SetDownMsgFun(const std::function<void(const caf::down_msg&,
                                              const std::string&)>& downMsgFun);

 private:
  std::mutex mapLock;
  std::function<void(const caf::down_msg& down_msg,
                     const std::string& description)>
      down_msg_fun;
  std::map<std::string, std::string> actor_map_;
  void AddMoMonitored(std::string actor_unity, std::string description);
  void DownMsgHandle(const caf::down_msg& down_msg,
                     const std::string& description);
};

bool SetMonitor(caf::actor& supervisor, caf::actor& worker,
                const std::string& description);

#endif  // ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
