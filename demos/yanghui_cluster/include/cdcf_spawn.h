/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_CDCF_SPAWN_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_CDCF_SPAWN_H_

#include "cdcf/actor_status_monitor.h"

class CdcfSpawn : public caf::event_based_actor {
 public:
  explicit CdcfSpawn(caf::actor_config& cfg);
  CdcfSpawn(caf::actor_config& cfg, cdcf::ActorStatusMonitor* monitor);

  caf::behavior make_behavior() override;

 private:
  cdcf::ActorStatusMonitor* monitor_;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_CDCF_SPAWN_H_
