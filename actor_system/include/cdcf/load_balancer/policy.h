/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_POLICY_H_
#define ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_POLICY_H_
#include <utility>
#include <vector>

#include <caf/all.hpp>

namespace cdcf::load_balancer {
struct Metrics {
  size_t load;
};

using Policy = std::function<std::pair<caf::actor, caf::mailbox_element_ptr>(
    const std::vector<caf::actor> &actors, const std::vector<Metrics> &metrics,
    caf::mailbox_element_ptr &mail)>;

namespace policy {

static constexpr const size_t kDefaultLoadThreshold{10};

Policy MinLoad(size_t load_threshold_to_hold = kDefaultLoadThreshold);
}  // namespace policy
}  // namespace cdcf::load_balancer

#endif  // ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_POLICY_H_
