/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_MESSAGE_PRIORITY_ACTOR_H
#define CDCF_MESSAGE_PRIORITY_ACTOR_H

#include <caf/all.hpp>

using high_priority_atom = caf::atom_constant<caf::atom("high_pri")>;
using normal_priority_atom = caf::atom_constant<caf::atom("normal_pri")>;

class MessagePriorityActor : public caf::event_based_actor {
 public:
  MessagePriorityActor(caf::actor_config& cfg) : event_based_actor(cfg){};
  void enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit* eu) override;

 private:
  bool IsFirstElementHighPriorityAtom(const caf::message& message);
  bool IsFirstElementNormalPriorityAtom(const caf::message& message);
  void DeleteFirstElement(caf::message& message);
  void AddMessageIdWithHighPriority(caf::message_id& id);
};

#endif  // CDCF_MESSAGE_PRIORITY_ACTOR_H
