/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
#include <condition_variable>

#include <caf/all.hpp>

#include "./balance_count_cluster.h"
#include "./priority_actor.h"

calculator::behavior_type calculator_fun(calculator::pointer self);
calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
