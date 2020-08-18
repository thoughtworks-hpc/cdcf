/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef MESSAGE_PRIORITY_ACTOR_INCLUDE_MESSAGE_PRIORITY_ACTOR_H_
#define MESSAGE_PRIORITY_ACTOR_INCLUDE_MESSAGE_PRIORITY_ACTOR_H_

#include <caf/all.hpp>

namespace cdcf::actor_system {
using high_priority_atom = caf::atom_constant<caf::atom("high_pri")>;
using normal_priority_atom = caf::atom_constant<caf::atom("normal_pri")>;

class MessagePriorityActor : public caf::event_based_actor {
 public:
  explicit MessagePriorityActor(caf::actor_config& cfg)
      : event_based_actor(cfg) {}
  void enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit* eu) override;

 private:
  bool IsFirstElementHighPriorityAtom(const caf::message& message);
  bool IsFirstElementNormalPriorityAtom(const caf::message& message);
  void DeleteFirstElement(caf::message& message);
  void AddMessageIdWithHighPriority(caf::message_id& id);
};
}  // namespace cdcf::actor_system

#endif  // MESSAGE_PRIORITY_ACTOR_INCLUDE_MESSAGE_PRIORITY_ACTOR_H_
