/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_

#include <actor_system.h>

#include "../counter_interface.h"

caf::behavior yanghui(caf::event_based_actor* self, counter_interface* counter);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_
