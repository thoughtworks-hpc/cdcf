/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_YANGHUI_SERVER_H
#define CDCF_YANGHUI_SERVER_H

#include <caf/all.hpp>

using yanghui_job_priority_actor = caf::typed_actor<
    caf::replies_to<std::vector<std::vector<int>>>::with<bool>>;

yanghui_job_priority_actor::behavior_type yanghui_job_priority_actor_fun(
    WorkerPool& worker_pool, caf::actor& dispatcher);

#endif  // CDCF_YANGHUI_SERVER_H
