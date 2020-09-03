/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_SRC_LOAD_BALANCER_PROXY_H_
#define ACTOR_SYSTEM_SRC_LOAD_BALANCER_PROXY_H_
#include <cdcf/logger.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include <caf/all.hpp>

#include "src/load_balancer/ticket.h"

namespace cdcf::load_balancer {
using Lock = caf::upgrade_lock<caf::detail::shared_spinlock>;

struct CustomMessageIdEquals {
  bool operator()(const caf::message_id &lhs,
                  const caf::message_id &rhs) const {
    return lhs.integer_value() == rhs.integer_value();
  }
};

class Proxy {
 public:
  void Relay(const caf::strong_actor_ptr &sender,
             caf::mailbox_element_ptr &what, caf::execution_unit *host,
             Lock &guard, caf::actor worker) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> lock{guard};
    auto new_id = PlaceTicket(what);
    lock.unlock();
    // replace sender with load_balancer
    worker->enqueue(sender, new_id, what->move_content_to_message(), host);
    return;
  }

  void Reply(const caf::strong_actor_ptr &sender,
             caf::mailbox_element_ptr &what, caf::execution_unit *host,
             Lock &guard, const std::vector<caf::actor> &workers) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> lock{guard};
    auto ticket = ExtractTicket(what, workers);
    lock.unlock();
    ticket.Reply(sender, what->move_content_to_message(), host);
  }

 private:
  caf::message_id PlaceTicket(caf::mailbox_element_ptr &what) {
    auto new_message_id = AllocateRequestID(what);
    auto ticket = Ticket::ReplyTo(what);
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    tickets_.emplace(new_message_id.response_id(), ticket);
    return new_message_id;
  }

  Ticket ExtractTicket(caf::mailbox_element_ptr &what,
                       const std::vector<caf::actor> &workers) {
    std::lock_guard<std::mutex> lock(ticket_mutex_);
    auto it = tickets_.find(what->mid);
    if (it == tickets_.end()) {
      return {};
    }
    /* worker get remove from load balancer, should remove the ticket */
    if (std::find(workers.begin(), workers.end(), what->sender) ==
        workers.end()) {
      tickets_.erase(it);
      return {};
    }
    auto result = it->second;
    tickets_.erase(it);
    return result;
  }

  caf::message_id AllocateRequestID(caf::mailbox_element_ptr &what) {
    auto priority = static_cast<caf::message_priority>(what->mid.category());
    auto result = ++last_request_id_;
    CDCF_LOGGER_DEBUG("current request id: {}", result.integer_value());
    return priority == caf::message_priority::normal
               ? result
               : result.with_high_priority();
  }

 private:
  caf::message_id last_request_id_ = {caf::make_message_id()};
  std::unordered_map<caf::message_id, Ticket, std::hash<caf::message_id>,
                     CustomMessageIdEquals>
      tickets_;
  std::mutex ticket_mutex_;
};
}  // namespace cdcf::load_balancer
#endif  // ACTOR_SYSTEM_SRC_LOAD_BALANCER_PROXY_H_
