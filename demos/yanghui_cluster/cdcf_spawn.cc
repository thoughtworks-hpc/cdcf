/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/cdcf_spawn.h"

CdcfSpawn::CdcfSpawn(caf::actor_config& cfg, cdcf::ActorStatusMonitor* monitor)
    : event_based_actor(cfg) {
  monitor_ = monitor;
}

CdcfSpawn::CdcfSpawn(caf::actor_config& cfg) : event_based_actor(cfg) {
  monitor_ = nullptr;
}

caf::behavior CdcfSpawn::make_behavior() {
  return {
      [&](caf::spawn_atom, std::string& actor_type, caf::message& actor_args,
          const caf::actor_system::mpi& actor_ifs, std::string& reg_name,
          std::string& reg_description) -> caf::actor {
        if (auto res = system().spawn<caf::actor>(actor_type, actor_args,
                                                  nullptr, true, &actor_ifs)) {
          if (monitor_ != nullptr) {
            monitor_->RegisterActor(*res, reg_name, reg_description);
          }
          return *res;
        }
        return nullptr;
      },
      [&](caf::spawn_atom, std::string& actor_type, caf::message& actor_args,
          const caf::actor_system::mpi& actor_ifs) -> caf::actor {
        if (auto res = system().spawn<caf::actor>(actor_type, actor_args,
                                                  nullptr, true, &actor_ifs)) {
          return *res;
        }
        return nullptr;
      }};
}
