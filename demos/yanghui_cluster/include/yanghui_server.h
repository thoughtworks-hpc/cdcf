/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_YANGHUI_SERVER_H
#define CDCF_YANGHUI_SERVER_H

#include <caf/all.hpp>

using yanghui_job_actor = caf::typed_actor<
    caf::replies_to<std::vector<std::vector<int>>>::with<bool>>;

yanghui_job_actor::behavior_type yanghui_priority_job_actor_fun(
    yanghui_job_actor::pointer self, WorkerPool& worker_pool,
    caf::actor& dispatcher);

yanghui_job_actor::behavior_type yanghui_standard_job_actor_fun(
    yanghui_job_actor::pointer self, ActorGuard& actor_guard);

#endif  // CDCF_YANGHUI_SERVER_H
