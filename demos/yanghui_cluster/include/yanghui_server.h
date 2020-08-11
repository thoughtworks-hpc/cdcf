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

const uint16_t yanghui_job1_port = 55011;
const uint16_t yanghui_job2_port = 55012;
const uint16_t yanghui_job3_port = 55013;
const uint16_t yanghui_job4_port = 55014;

const std::string yanghui_job1_name = "yanghui_standard_job";
const std::string yanghui_job2_name = "yanghui_priority_job";
const std::string yanghui_job3_name = "yanghui_load_balance_job";
const std::string yanghui_job4_name = "yanghui_router_pool_job";

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
