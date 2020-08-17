/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "cdcf/actor_system/load_balancer.h"

#include <caf/default_attachable.hpp>

#include "src/load_balancer/proxy.h"

namespace cdcf::load_balancer {
Router::Router(caf::actor_config &config)
    : caf::monitorable_actor(config),
      planned_reason_(caf::exit_reason::normal),
      proxy_(std::make_unique<Proxy>()) {}

caf::actor Router::Make(caf::execution_unit *host,
                        load_balancer::Policy &&policy) {
  auto &sys = host->system();
  caf::actor_config config{host};
  auto res = caf::make_actor<Router, caf::actor>(sys.next_actor_id(),
                                                 sys.node(), &sys, config);
  auto abstract = caf::actor_cast<caf::abstract_actor *>(res);
  auto ptr = static_cast<Router *>(abstract);
  ptr->policy_ = std::move(policy);
  return res;
}

void Router::on_destroy() {
  CAF_PUSH_AID_FROM_PTR(this);
  if (!getf(is_cleaned_up_flag)) {
    cleanup(caf::exit_reason::unreachable, nullptr);
    monitorable_actor::on_destroy();
    unregister_from_system();
  }
}

void Router::on_cleanup(const caf::error &reason) {
  CAF_PUSH_AID_FROM_PTR(this);
  CAF_IGNORE_UNUSED(reason);
  CAF_LOG_TERMINATE_EVENT(this, reason);
}

void Router::enqueue(caf::mailbox_element_ptr what, caf::execution_unit *host) {
  Lock guard{lock_};
  if (Filter(guard, what, host)) {
    return;
  }
  const auto is_response = what->mid.is_response();
  if (is_response) {
    auto from = caf::actor_cast<caf::actor>(what->sender);
    --metrics_[from].load;
    proxy_->Reply(ctrl(), what, host, guard, workers_);
  }
  auto this_mail = is_response ? nullptr : std::move(what);
  auto [to, next_mail] = policy_(workers_, GetMetrics(), this_mail);
  if (to && next_mail) {
    ++metrics_[to].load;
    proxy_->Relay(ctrl(), next_mail, host, guard, to);
  }
}

bool Router::Filter(Lock &guard, caf::mailbox_element_ptr &what,
                    caf::execution_unit *eu) {
  const auto &sender = what->sender;
  const auto &mid = what->mid;
  const auto &content = what->content();
  CAF_LOG_TRACE(CAF_ARG(mid) << CAF_ARG(content));
  if (content.match_elements<caf::exit_msg>()) {
    Exit(guard, eu, content.get_as<caf::exit_msg>(0));
    return true;
  }
  if (content.match_elements<caf::down_msg>()) {
    Down(guard, eu, content.get_as<caf::down_msg>(0));
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::put_atom, caf::actor>()) {
    AddWorker(guard, content.get_as<caf::actor>(2));
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::delete_atom, caf::actor>()) {
    DeleteWorker(guard, content.get_as<caf::actor>(2));
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::delete_atom>()) {
    ClearWorker(guard);
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::get_atom>()) {
    auto workers = workers_;
    guard.unlock();
    sender->enqueue(nullptr, mid.response_id(),
                    make_message(std::move(workers)), eu);
    return true;
  }
  if (mid.is_request() && sender != nullptr && workers_.empty()) {
    guard.unlock();
    /* Tell client we have ignored this request message by sending and empty
     * message back. */
    sender->enqueue(nullptr, mid.response_id(), caf::message{}, eu);
    return true;
  }
  return false;
}

void Router::AddWorker(Lock &guard, const caf::actor &worker) {
  auto it = std::find(workers_.begin(), workers_.end(), worker);
  if (it != workers_.end()) {
    return;
  }
  worker->attach(
      caf::default_attachable::make_monitor(worker.address(), address()));
  caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{guard};
  workers_.push_back(worker);
  metrics_.emplace(worker, Metrics{});
}

void Router::DeleteWorker(Lock &guard, const caf::actor &worker) {
  caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{guard};
  auto last = workers_.end();
  auto it = std::find(workers_.begin(), last, worker);
  if (it != last) {
    caf::default_attachable::observe_token token{
        address(), caf::default_attachable::monitor};
    worker->detach(token);
    workers_.erase(it);
    metrics_.erase(worker);
  }
}

void Router::ClearWorker(Lock &guard) {
  caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{guard};
  for (auto &worker : workers_) {
    caf::default_attachable::observe_token token{
        address(), caf::default_attachable::monitor};
    worker->detach(token);
  }
  workers_.clear();
  metrics_.clear();
}

void Router::Exit(Lock &guard, caf::execution_unit *eu,
                  const caf::exit_msg &exit_message) {
  std::vector<caf::actor> workers;
  std::unordered_map<caf::actor, Metrics> metrics;
  auto reason = exit_message.reason;
  if (cleanup(std::move(reason), eu)) {
    // send exit messages *always* to all workers and clear vector
    // afterwards but first swap workers_ out of the critical section
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    workers_.swap(workers);
    metrics_.swap(metrics);
    unique_guard.unlock();
    const auto message = make_message(exit_message);
    for (const auto &worker : workers) {
      anon_send(worker, message);
    }
    unregister_from_system();
  }
}

void Router::Down(Lock &guard, caf::execution_unit *eu,
                  const caf::down_msg &down_message) {
  // remove failed worker from pool
  caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{guard};
  auto last = workers_.end();
  auto i = std::find(workers_.begin(), workers_.end(), down_message.source);
  CAF_LOG_DEBUG_IF(i == last, "received down message for an unknown worker");
  if (i != last) {
    metrics_.erase(*i);
    workers_.erase(i);
  }
  if (workers_.empty()) {
    planned_reason_ = caf::exit_reason::out_of_workers;
    unique_guard.unlock();
    // we can safely run our cleanup code here without holding
    // lock_ because abstract_actor has its own lock
    if (cleanup(planned_reason_, eu)) {
      unregister_from_system();
    }
  }
}
}  // namespace cdcf::load_balancer
