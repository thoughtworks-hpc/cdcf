/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_

#include <vector>

#include <caf/all.hpp>

#include "cdcf/actor_guard.h"
#include "../yanghui_config.h"
#include "./yanghui_with_priority.h"

static const uint16_t yanghui_job1_port = 55021;
static const uint16_t yanghui_job2_port = 55022;
static const uint16_t yanghui_job3_port = 55023;
static const uint16_t yanghui_job4_port = 55024;

static const char yanghui_job1_name[] = "yanghui_standard_job";
static const char yanghui_job2_name[] = "yanghui_priority_job";
static const char yanghui_job3_name[] = "yanghui_load_balance_job";
static const char yanghui_job4_name[] = "yanghui_router_pool_job";

struct yanghui_job_state {
  caf::strong_actor_ptr message_sender;
};

/**
 *  yanghui_standard_job_actor
 */
caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, cdcf::actor_system::ActorGuard* actor_guard);

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
    caf::stateful_actor<yanghui_job_state>* self, cdcf::actor_system::ActorGuard* pool_guard);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
