/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_

#include <vector>

#include <caf/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "../yanghui_config.h"
#include "./yanghui_with_priority.h"

const uint16_t yanghui_job_port1 = 55011;
const uint16_t yanghui_job_port2 = 55012;
const uint16_t yanghui_job_port3 = 55013;
const uint16_t yanghui_job_port4 = 55014;

struct yanghui_job_state {
  caf::strong_actor_ptr message_sender;
};

/**
 *  yanghui_standard_job_actor
 */
caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* actor_guard);

/**
 *  yanghui_priority_job_actor
 */
caf::behavior yanghui_priority_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, WorkerPool* worker_pool,
    caf::actor dispatcher);

/**
 *  yanghui_load_balance_job_actor
 */
caf::behavior yanghui_load_balance_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    caf::actor yanghui_load_balance_count_path,
    caf::actor yanghui_load_balance_get_min);
/**
 *  yanghui_router_pool_job_actor
 */
caf::behavior yanghui_router_pool_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* pool_guard);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
