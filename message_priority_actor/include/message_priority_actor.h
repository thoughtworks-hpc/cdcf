/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_MESSAGE_PRIORITY_ACTOR_H
#define CDCF_MESSAGE_PRIORITY_ACTOR_H

#include <caf/all.hpp>

class MessagePriorityActor : public caf::event_based_actor {
 public:
  MessagePriorityActor(caf::actor_config& cfg) : event_based_actor(cfg){};
  void enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit* eu) override;

 private:
  bool IsFirstElementString(const caf::message& message);
  bool IsFirstElementInHighOrNormalPriority(const caf::message& message,
                                            bool& is_high_priority);
  void DeleteFirstElement(caf::message& message);
  void AddMessageIdWithHighPriority(caf::message_id& id);
};

#endif  // CDCF_MESSAGE_PRIORITY_ACTOR_H
