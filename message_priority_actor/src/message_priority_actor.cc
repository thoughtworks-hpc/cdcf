/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "../include/message_priority_actor.h"

void MessagePriorityActor::enqueue(caf::mailbox_element_ptr ptr,
                                   caf::execution_unit* eu) {
  auto message = ptr->copy_content_to_message();
  auto message_id = ptr->mid;

  if (IsFirstElementHighPriorityAtom(message)) {
    DeleteFirstElement(message);
    AddMessageIdWithHighPriority(message_id);
    auto element =
        make_mailbox_element(ptr->sender, message_id, ptr->stages, message);
    scheduled_actor::enqueue(std::move(element), eu);
  } else if (IsFirstElementNormalPriorityAtom(message)) {
    DeleteFirstElement(message);
    auto element =
        make_mailbox_element(ptr->sender, message_id, ptr->stages, message);
    scheduled_actor::enqueue(std::move(element), eu);
  } else {
    scheduled_actor::enqueue(std::move(ptr), eu);
  }
}

bool MessagePriorityActor::IsFirstElementHighPriorityAtom(
    const caf::message& message) {
  return message.match_element<high_priority_atom>(0);
}

bool MessagePriorityActor::IsFirstElementNormalPriorityAtom(
    const caf::message& message) {
  return message.match_element<normal_priority_atom>(0);
}

void MessagePriorityActor::DeleteFirstElement(caf::message& message) {
  message = message.drop(1);
}

void MessagePriorityActor::AddMessageIdWithHighPriority(caf::message_id& id) {
  id = id.with_high_priority();
}
