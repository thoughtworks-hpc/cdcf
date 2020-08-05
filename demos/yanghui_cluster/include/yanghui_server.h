/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_

#include <vector>

#include <caf/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "./yanghui_with_priority.h"

const uint16_t yanghui_job_port1 = 55011;
const uint16_t yanghui_job_port2 = 55012;
const uint16_t yanghui_job_port3 = 55013;
const uint16_t yanghui_job_port4 = 55014;

struct yanghui_job_state {
  caf::strong_actor_ptr message_sender;
};

struct YanghuiData {
  std::vector<std::vector<int>> data;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, const YanghuiData& x) {
  return f(caf::meta::type_name("YanghuiData"), x.data);
}

caf::behavior mirror(caf::event_based_actor* self);

/**
 *  yanghui_standard_job_actor
 */
// using yanghui_standard_job_actor =
//    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
//                     caf::reacts_to<caf::strong_actor_ptr, int>>;

caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* actor_guard);

/**
 *  yanghui_priority_job_actor
 */

// using yanghui_priority_job_actor =
//    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
//                     caf::reacts_to<std::vector<std::pair<bool, int>>>>;

caf::behavior yanghui_priority_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, WorkerPool* worker_pool,
    caf::actor dispatcher);

/**
 *  yanghui_load_balance_job_actor
 */

// using yanghui_load_balance_job_actor =
//    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
//                     caf::reacts_to<std::vector<int>>, caf::reacts_to<int>>;

caf::behavior yanghui_load_balance_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    caf::actor yanghui_load_balance_count_path,
    caf::actor yanghui_load_balance_get_min);
/**
 *  yanghui_router_pool_job_actor
 */
// using yanghui_router_pool_job_actor =
//    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
//                     caf::reacts_to<caf::strong_actor_ptr, int>>;

caf::behavior yanghui_router_pool_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* pool_guard);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_SERVER_H_
