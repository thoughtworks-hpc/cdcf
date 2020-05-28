/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_SRC_LOAD_BALANCER_TICKET_H_
#define ACTOR_SYSTEM_SRC_LOAD_BALANCER_TICKET_H_
#include <caf/all.hpp>

namespace cdcf::load_balancer {
struct Ticket {
  static Ticket ReplyTo(caf::mailbox_element_ptr &what) {
    bool required_reply = what->mid != what->mid.response_id();
    return required_reply ? Ticket{what->mid.response_id(),
                                   caf::actor_cast<caf::actor>(what->sender)}
                          : Ticket{};
  }

  caf::message_id response_id;
  caf::actor response_to;

  void Reply(caf::strong_actor_ptr sender, const caf::message &message,
             caf::execution_unit *host) const {
    if (response_to) {
      response_to->enqueue(sender, response_id, message, host);
    }
  }
};
}  // namespace cdcf::load_balancer
#endif  // ACTOR_SYSTEM_SRC_LOAD_BALANCER_TICKET_H_
