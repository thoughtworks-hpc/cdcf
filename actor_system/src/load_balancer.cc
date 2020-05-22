/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "actor_system/load_balancer.h"

namespace cdcf::load_balancer {
std::pair<caf::message_id, Ticket> Router::PlaceTicket(
    caf::mailbox_element_ptr &what) {
  auto new_message_id = AllocateRequestID(what);
  auto ticket = Ticket::ReplyTo(what);
  tickets_.emplace(new_message_id.response_id(), ticket);
  return std::make_pair(new_message_id, ticket);
}

Ticket Router::ExtractTicket(caf::mailbox_element_ptr &what) {
  auto it = tickets_.find(what->mid);
  if (it == tickets_.end()) {
    return {};
  }
  /* worker get remove from load balancer, should remove the ticket */
  if (std::find(workers_.begin(), workers_.end(), what->sender) ==
      workers_.end()) {
    tickets_.erase(it);
    return {};
  }
  auto result = it->second;
  tickets_.erase(it);
  return result;
}

caf::message_id Router::AllocateRequestID(caf::mailbox_element_ptr &what) {
  auto priority = static_cast<caf::message_priority>(what->mid.category());
  auto result = ++last_request_id_;
  return priority == caf::message_priority::normal
             ? result
             : result.with_high_priority();
}

void Router::enqueue(caf::mailbox_element_ptr what, caf::execution_unit *host) {
  Lock guard{workers_mtx_};
  if (Filter(guard, what, host)) {
    return;
  }
  if (what->mid.is_response()) {
    return Reply(what, host, guard);
  } else {
    return Relay(what, host, guard);
  }
}

void Router::Relay(caf::mailbox_element_ptr &what, caf::execution_unit *host,
                   Lock &guard) {
  auto worker = policy_(workers_, GetMetrics(), what);
  ++metrics_[worker].load;
  auto [new_id, _] = PlaceTicket(what);
  guard.unlock();
  // replace sender with load_balancer
  worker->enqueue(ctrl(), new_id, what->move_content_to_message(), host);
  return;
}

void Router::Reply(caf::mailbox_element_ptr &what, caf::execution_unit *host,
                   Lock &guard) {
  auto from = caf::actor_cast<caf::actor>(what->sender);
  --metrics_[from].load;
  auto ticket = ExtractTicket(what);
  guard.unlock();
  ticket.Reply(ctrl(), what->move_content_to_message(), host);
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
    caf::message_id mid1 = mid.response_id();
    sender->enqueue(nullptr, mid1, make_message(std::move(workers)), eu);
    return true;
  }
  if (mid.is_request() && sender != nullptr && workers_.empty()) {
    guard.unlock();
    caf::message_id mid1 =
        mid.response_id();  // Tell client we have ignored this request message
                            // by sending and empty
                            // message back.
    sender->enqueue(nullptr, mid1, caf::message{}, eu);
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
  auto i = std::find(workers_.begin(), last, worker);
  if (i != last) {
    caf::default_attachable::observe_token tk{address(),
                                              caf::default_attachable::monitor};
    worker->detach(tk);
    workers_.erase(i);
    metrics_.erase(worker);
  }
}

void Router::ClearWorker(Lock &guard) {
  caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{guard};
  for (auto &worker : workers_) {
    caf::default_attachable::observe_token tk{address(),
                                              caf::default_attachable::monitor};
    worker->detach(tk);
  }
  workers_.clear();
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
    // workers_mtx_ because abstract_actor has its own lock
    if (cleanup(planned_reason_, eu)) {
      unregister_from_system();
    }
  }
}
}  // namespace cdcf::load_balancer
