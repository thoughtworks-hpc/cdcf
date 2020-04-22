/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_MESSAGE_GUARANTOR_H_
#define ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_MESSAGE_GUARANTOR_H_
#include <map>
#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

using MessageID = unsigned int;

class ActorMessageGuarantor {
 public:
  explicit ActorMessageGuarantor(caf::actor receiver, caf::actor_system& system)
      : receiver_(std::move(receiver)), sender_actor_(system) {}

  void SendMsg(MessageID msg_id, const caf::message& msg);
  void ConfirmMsg(MessageID msg_id);
  void ReSendMsg();
  void SetReceiver(const caf::actor& receiver);

  template <class... Ts>
  void Send(MessageID msg_id, const Ts&... xs) {
    auto send_msg = caf::make_message(xs...);
    SendMsg(msg_id, send_msg);
  }

  template <class... send, class return_function, class error_handle>
  void SendAndReceive(MessageID msg_id, return_function f, error_handle e,
                      const send&... xs) {
    sender_actor_->request(receiver_, std::chrono::seconds(1), xs...)
        .receive(f, e);
  }

 private:
  caf::actor receiver_;
  std::map<MessageID, caf::message> messages_in_processing_;
  std::mutex buffer_locker_;
  caf::scoped_actor sender_actor_;
};

#endif  // ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_MESSAGE_GUARANTOR_H_
