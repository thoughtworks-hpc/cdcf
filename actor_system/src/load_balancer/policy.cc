/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "actor_system/load_balancer/policy.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <queue>

namespace cdcf::load_balancer::policy {
class MinLoadImpl {
 public:
  explicit MinLoadImpl(size_t load_threshold_to_hold = kMaxLoad)
      : load_threshold_to_hold_(load_threshold_to_hold) {}

  MinLoadImpl(MinLoadImpl &&other) noexcept
      : rotated_(other.rotated_),
        mails_(std::move(other.mails_)),
        load_threshold_to_hold_(other.load_threshold_to_hold_) {}

  MinLoadImpl &operator=(MinLoadImpl &&other) noexcept {
    rotated_ = other.rotated_;
    load_threshold_to_hold_ = other.load_threshold_to_hold_;
    mails_ = std::move(other.mails_);
    return *this;
  }

  std::pair<caf::actor, caf::mailbox_element_ptr> operator()(
      const std::vector<caf::actor> &actors,
      const std::vector<Metrics> &metrics, caf::mailbox_element_ptr &mail) {
    assert(actors.size() == metrics.size());
    std::lock_guard lock{mutex_};
    if (!mail && mails_.empty()) {
      return {};
    }

    auto rotated = Rotate(metrics);
    auto less_load = [](auto &a, auto &b) { return a.load < b.load; };
    auto selected = std::min_element(rotated.begin(), rotated.end(), less_load);
    if (selected->load >= load_threshold_to_hold_) {
      Hold(mail);
      return {};
    }

    Release(mail);
    auto offset = selected - rotated.begin();
    return {actors[(rotated_ + offset) % actors.size()], std::move(mail)};
  }

 private:
  std::vector<Metrics> Rotate(const std::vector<Metrics> &metrics) {
    rotated_ = (rotated_ + 1) % metrics.size();
    std::vector<Metrics> result{metrics.size()};
    auto pivot = metrics.begin() + rotated_;
    /* round robin with std::rotated, note its complexity is O(n). */
    std::rotate_copy(metrics.begin(), pivot, metrics.end(), result.begin());
    return result;
  }

  void Hold(caf::mailbox_element_ptr &mail) {
    if (mail) {
      auto m = make_mailbox_element(mail->sender, mail->mid, mail->stages,
                                    mail->copy_content_to_message());
      mails_.emplace(std::move(m));
    }
  }

  void Release(caf::mailbox_element_ptr &mail) {
    if (mail) {
      mails_.emplace(std::move(mail));
    }
    mail = std::move(mails_.front());
    mails_.pop();
  }

  static constexpr const size_t kMaxLoad{10};
  size_t rotated_{std::numeric_limits<size_t>::max()};
  std::queue<caf::mailbox_element_ptr> mails_;
  size_t load_threshold_to_hold_;
  std::mutex mutex_;
};

Policy MinLoad() {
  auto i = std::make_shared<MinLoadImpl>();
  return [i](const std::vector<caf::actor> &actors,
             const std::vector<Metrics> &metrics,
             caf::mailbox_element_ptr &mail) mutable {
    return (*i)(actors, metrics, mail);
  };
}
}  // namespace cdcf::load_balancer::policy
