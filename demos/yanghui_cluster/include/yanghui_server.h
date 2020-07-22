/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_YANGHUI_SERVER_H
#define CDCF_YANGHUI_SERVER_H

#include <caf/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "./yanghui_with_priority.h"

using yanghui_priority_job_actor = caf::typed_actor<
    caf::reacts_to<std::vector<std::vector<int>>>,
    caf::replies_to<std::vector<std::pair<bool, int>>>::with<bool>>;

struct yanghui_priority_job_state {
  caf::strong_actor_ptr referer;
};

yanghui_priority_job_actor::behavior_type yanghui_priority_job_actor_fun(
    yanghui_priority_job_actor::stateful_pointer<yanghui_priority_job_state>
        self,
    WorkerPool* worker_pool, caf::actor dispatcher);

using yanghui_standard_job_actor =
    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
                     caf::reacts_to<int>>;

yanghui_standard_job_actor::behavior_type yanghui_standard_job_actor_fun(
    yanghui_standard_job_actor::pointer self, ActorGuard* actor_guard);

using yanghui_compare_job_actor =
    caf::typed_actor<caf::replies_to<std::vector<std::vector<int>>>::with<int>>;

yanghui_compare_job_actor::behavior_type yanghui_compare_job_actor_fun(
    yanghui_compare_job_actor::pointer self);

#endif  // CDCF_YANGHUI_SERVER_H
