/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "actor_system/load_balancer.h"

#include <caf/default_attachable.hpp>

namespace cdcf {
bool load_balancer::Filter(
    caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
    const caf::strong_actor_ptr &sender, caf::message_id mid,
    caf::message_view &mv, caf::execution_unit *eu) {
  auto &content = mv.content();
  CAF_LOG_TRACE(CAF_ARG(mid) << CAF_ARG(content));
  if (content.match_elements<caf::exit_msg>()) {
    // acquire second mutex as well
    std::vector<caf::actor> workers;
    auto em = content.get_as<caf::exit_msg>(0).reason;
    if (cleanup(std::move(em), eu)) {
      auto tmp = mv.move_content_to_message();
      // send exit messages *always* to all workers and clear vector
      // afterwards but first swap workers_ out of the critical section
      caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
          guard};
      workers_.swap(workers);
      unique_guard.unlock();
      for (auto &w : workers) anon_send(w, tmp);
      unregister_from_system();
    }
    return true;
  }
  if (content.match_elements<caf::down_msg>()) {
    // remove failed worker from pool
    auto &dm = content.get_as<caf::down_msg>(0);
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    auto last = workers_.end();
    auto i = std::find(workers_.begin(), workers_.end(), dm.source);
    CAF_LOG_DEBUG_IF(i == last, "received down message for an unknown worker");
    if (i != last) workers_.erase(i);
    if (workers_.empty()) {
      planned_reason_ = caf::exit_reason::out_of_workers;
      unique_guard.unlock();
      Quit(eu);
    }
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::put_atom, caf::actor>()) {
    auto &worker = content.get_as<caf::actor>(2);
    worker->attach(
        caf::default_attachable::make_monitor(worker.address(), address()));
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    workers_.push_back(worker);
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::delete_atom, caf::actor>()) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    auto &what = content.get_as<caf::actor>(2);
    auto last = workers_.end();
    auto i = std::find(workers_.begin(), last, what);
    if (i != last) {
      caf::default_attachable::observe_token tk{
          address(), caf::default_attachable::monitor};
      what->detach(tk);
      workers_.erase(i);
    }
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::delete_atom>()) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    for (auto &worker : workers_) {
      caf::default_attachable::observe_token tk{
          address(), caf::default_attachable::monitor};
      worker->detach(tk);
    }
    workers_.clear();
    return true;
  }
  if (content.match_elements<caf::sys_atom, caf::get_atom>()) {
    auto cpy = workers_;
    guard.unlock();
    sender->enqueue(nullptr, mid.response_id(), make_message(std::move(cpy)),
                    eu);
    return true;
  }
  if (workers_.empty()) {
    guard.unlock();
    if (mid.is_request() && sender != nullptr) {
      // Tell client we have ignored this request message by sending and empty
      // message back.
      sender->enqueue(nullptr, mid.response_id(), caf::message{}, eu);
    }
    return true;
  }
  return false;
}
}  // namespace cdcf
