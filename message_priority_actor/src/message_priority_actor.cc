/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "../include/message_priority_actor.h"

void MessagePriorityActor::enqueue(caf::mailbox_element_ptr ptr,
                                   caf::execution_unit *eu) {
  auto message = ptr->copy_content_to_message();
  auto message_id = ptr->mid;

  if (message.match_element<std::string>(0)) {
    auto priority = message.get_as<std::string>(0);
    if (priority == "high priority" || priority == "normal priority") {
      if (priority == "high priority") {
        message_id = ptr->mid.with_high_priority();
      }
      message = message.drop(1);
      auto element =
          make_mailbox_element(ptr->sender, message_id, ptr->stages, message);
      scheduled_actor::enqueue(std::move(element), eu);
    }
  } else {
    scheduled_actor::enqueue(std::move(ptr), eu);
  }
}