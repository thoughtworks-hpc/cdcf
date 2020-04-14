/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "actor_message_guarantor.h"

void ActorMessageGuarantor::SendMsg(MessageID msg_id, const caf::message& msg) {
  {
    std::lock_guard<std::mutex> locker(buffer_locker_);
    messages_in_processing_[msg_id] = msg;
  }

  caf::anon_send(receiver_, msg);
}

void ActorMessageGuarantor::ConfirmMsg(MessageID msg_id) {
  std::lock_guard<std::mutex> locker(buffer_locker_);
  messages_in_processing_.erase(msg_id);
}

void ActorMessageGuarantor::ReSendMsg() {
  std::lock_guard<std::mutex> locker(buffer_locker_);
  for (auto& msg : messages_in_processing_) {
    caf::anon_send(receiver_, msg);
  }
}
void ActorMessageGuarantor::SetReceiver(const caf::actor& receiver) {
  receiver_ = receiver;
}
