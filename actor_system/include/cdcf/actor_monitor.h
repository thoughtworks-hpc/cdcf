/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
#define ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
#include <map>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

namespace cdcf {
class ActorMonitor : public caf::event_based_actor {
 public:
  using demonitor_atom = caf::atom_constant<caf::atom("demonitor")>;
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
  void DownMsgHandle(const caf::down_msg& down_msg,
                     const std::string& description);
};

bool SetMonitor(caf::actor& supervisor, caf::actor& worker,
                const std::string& description);

bool StopMonitor(caf::actor& supervisor, const caf::actor_addr& worker);
}  // namespace cdcf
#endif  // ACTOR_MONITOR_INCLUDE_ACTOR_MONITOR_H_
