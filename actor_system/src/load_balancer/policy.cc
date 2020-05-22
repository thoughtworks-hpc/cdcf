/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "actor_system/load_balancer/policy.h"

#include <algorithm>
#include <limits>

namespace cdcf::load_balancer::policy {
Policy MinLoad() {
  return [rotated_ = std::numeric_limits<size_t>::max()](
             const std::vector<caf::actor> &actors,
             const std::vector<Metrics> &metrics,
             caf::mailbox_element_ptr &mail) mutable {
    rotated_ = (rotated_ + 1) % actors.size();
    std::vector<Metrics> rotated{metrics.size()};
    auto pivot = metrics.begin() + rotated_;
    /* Implement round robin with std::rotated, note its complexity is O(n).
     */
    std::rotate_copy(metrics.begin(), pivot, metrics.end(), rotated.begin());

    auto min_load = [](auto &a, auto &b) { return a.load < b.load; };
    auto it = std::min_element(rotated.begin(), rotated.end(), min_load);
    auto offset = it - rotated.begin();
    return actors[(rotated_ + offset) % actors.size()];
  };
}
}  // namespace cdcf::load_balancer::policy
