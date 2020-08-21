/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_ACTOR_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_ACTOR_H_

#include <cdcf/actor_system.h>

#include "../counter_interface.h"

caf::behavior yanghui(caf::event_based_actor* self, counter_interface* counter);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_ACTOR_H_
