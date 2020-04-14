/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_ACTOR_GUARD_H
#define CDCF_ACTOR_GUARD_H
#include "../../actor_monitor/include/actor_monitor.h"
#include "./actor_message_guarantor.h"

class ActorGuard {
 public:
  ActorGuard(caf::actor& keepActor,
             std::function<caf::actor(std::atomic<bool>&)> restart,
             caf::actor_system& system);
  bool SendMsg(MessageID msg_id, const caf::message& msg);
  void ConfirmMsg(MessageID msg_id);

  template <class... Ts>
  bool Send(MessageID msg_id, const Ts&... xs) {
    if (active_) {
      message_guarantor_.Send(msg_id, xs...);
    }

    return active_;
  }

  template <class... send, class return_function>
  bool SendAndReceive(return_function f, const send&... xs) {
    if (active_) {
      caf::message send_message = caf::make_message(xs...);
      message_guarantor_.SendAndReceive(
          0, f, [&](caf::error) { HandleSendFailed(send_message, f); }, xs...);
    }

    return active_;
  }

 private:
  template <class return_function>
  void HandleSendFailed(const caf::message& msg, return_function f) {
    std::cout << "send  msg failed, try restart dest actor." << std::endl;
    message_guarantor_.SetReceiver(restart_fun_(active_));

    if (active_) {
      std::cout << "restart actor success." << std::endl;
      (void)SendAndReceive(f, msg);
    } else {
      std::cout << "restart actor failed. message:" << caf::to_string(msg)
                << " will not deliver." << std::endl;
    }
  }

  void HandleDownMsg(const std::string& actor_description);
  ActorMessageGuarantor message_guarantor_;
  std::function<caf::actor(std::atomic<bool>&)> restart_fun_;
  caf::actor monitor_;
  std::atomic<bool> active_ = true;
};

#endif  // CDCF_ACTOR_GUARD_H
