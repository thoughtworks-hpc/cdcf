/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_LOAD_BALANCER_H_
#define ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_LOAD_BALANCER_H_

#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>

#include <caf/all.hpp>

#include "./policy.h"

namespace cdcf::load_balancer {
using Lock = caf::upgrade_lock<caf::detail::shared_spinlock>;

class Proxy;

class Router : public caf::monitorable_actor {
 public:
  static caf::actor Make(caf::execution_unit *host,
                         load_balancer::Policy &&policy);

 public:
  explicit Router(caf::actor_config &config);

  void enqueue(caf::mailbox_element_ptr what,
               caf::execution_unit *host) override;

  void on_destroy() override;

 protected:
  void on_cleanup(const caf::error &reason) override;

 private:
  std::vector<Metrics> GetMetrics() const;

  bool Filter(Lock &guard, caf::mailbox_element_ptr &what,
              caf::execution_unit *eu);

  void ClearWorker(Lock &guard);

  void DeleteWorker(Lock &guard, const caf::actor &worker);

  void AddWorker(Lock &guard, const caf::actor &worker);

  void Down(Lock &guard, caf::execution_unit *eu, const caf::down_msg &dm);

  void Exit(Lock &guard, caf::execution_unit *eu, const caf::exit_msg &message);

  caf::detail::shared_spinlock lock_;
  caf::exit_reason planned_reason_;
  std::vector<caf::actor> workers_;
  std::unordered_map<caf::actor, Metrics> metrics_;
  std::unique_ptr<Proxy> proxy_;
  Policy policy_;
};

inline std::vector<Metrics> Router::GetMetrics() const {
  std::vector<Metrics> result(workers_.size());
  std::transform(workers_.begin(), workers_.end(), result.begin(),
                 [&](auto &worker) {
                   auto it = metrics_.find(worker);
                   assert(it != metrics_.end());
                   return it->second;
                 });
  return result;
}

}  // namespace cdcf::load_balancer
#endif  // ACTOR_SYSTEM_INCLUDE_CDCF_LOAD_BALANCER_LOAD_BALANCER_H_
