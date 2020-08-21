/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_SYSTEM_INCLUDE_CDCF_MESSAGE_PRIORITY_ACTOR_H_
#define ACTOR_SYSTEM_INCLUDE_CDCF_MESSAGE_PRIORITY_ACTOR_H_

#include <caf/all.hpp>

namespace cdcf {
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
}  // namespace cdcf

#endif  // ACTOR_SYSTEM_INCLUDE_CDCF_MESSAGE_PRIORITY_ACTOR_H_
