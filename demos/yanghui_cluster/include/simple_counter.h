/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_

#include "../yanghui_config.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

caf::behavior simple_counter(caf::event_based_actor* self);
caf::behavior simple_counter_add_load(caf::event_based_actor* self,
                                      int load_second);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_SIMPLE_COUNTER_H_
