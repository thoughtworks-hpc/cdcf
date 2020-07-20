/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "../include/message_priority_actor.h"

void MessagePriorityActor::enqueue(caf::mailbox_element_ptr ptr,
                                   caf::execution_unit* eu) {
  auto message = ptr->copy_content_to_message();
  auto message_id = ptr->mid;
  bool is_high_priority = false;

  if (IsFirstElementString(message) &&
      IsFirstElementInHighOrNormalPriority(message, is_high_priority)) {
    DeleteFirstElement(message);
    if (is_high_priority) {
      AddMessageIdWithHighPriority(message_id);
    }
    auto element =
        make_mailbox_element(ptr->sender, message_id, ptr->stages, message);
    scheduled_actor::enqueue(std::move(element), eu);
  } else {
    scheduled_actor::enqueue(std::move(ptr), eu);
  }
}

bool MessagePriorityActor::IsFirstElementString(const caf::message& message) {
  return message.match_element<std::string>(0);
}

bool MessagePriorityActor::IsFirstElementInHighOrNormalPriority(
    const caf::message& message, bool& is_high_priority) {
  auto priority = message.get_as<std::string>(0);
  if (priority == "high priority") {
    is_high_priority = true;
  }

  return (priority == "high priority" || priority == "normal priority");
}

void MessagePriorityActor::DeleteFirstElement(caf::message& message) {
  message = message.drop(1);
}

void MessagePriorityActor::AddMessageIdWithHighPriority(caf::message_id& id) {
  id = id.with_high_priority();
}