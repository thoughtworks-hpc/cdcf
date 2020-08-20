/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_
#define ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_

#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../logger/include/logger.h"

class ActorGuard {
 public:
  ActorGuard(caf::actor& keepActor,
             std::function<caf::actor(std::atomic<bool>&)> restart,
             caf::actor_system& system,
             std::chrono::seconds timeout_in_seconds = std::chrono::seconds(30))
      : keep_actor_(keepActor),
        restart_fun_(std::move(restart)),
        sender_actor_(system),
        timeout_in_seconds_(timeout_in_seconds) {}

  template <class... send_type, class return_function_type>
  bool SendAndReceive(return_function_type return_function,
                      std::function<void(caf::error)> error_deal_function,
                      const send_type&... messages) {
    if (active_) {
      caf::message send_message = caf::make_message(messages...);

      sender_actor_->request(keep_actor_, timeout_in_seconds_, messages...)
          .receive(return_function, [&](caf::error err) {
            HandleSendFailed(send_message, return_function, error_deal_function,
                             err);
          });
    }

    return active_;
  }

 private:
  template <class return_function_type>
  void HandleSendFailed(const caf::message& message,
                        return_function_type return_function,
                        std::function<void(caf::error)> error_deal_function,
                        caf::error err) {
    if ("system" != caf::to_string(err.category())) {
      // not system error, mean actor not down, this is a business error.
      error_deal_function(err);
      return;
    }

    CDCF_LOGGER_ERROR("send msg failed, try restart dest actor.");
    keep_actor_ = restart_fun_(active_);

    if (active_) {
      CDCF_LOGGER_INFO("restart actor success.");
      (void)SendAndReceive(return_function, error_deal_function, message);
    } else {
      CDCF_LOGGER_ERROR("restart actor failed. message:{} will not deliver.",
                        caf::to_string(message));
      error_deal_function(err);
    }
  }

  caf::actor keep_actor_;
  caf::scoped_actor sender_actor_;
  std::function<caf::actor(std::atomic<bool>&)> restart_fun_;
  std::atomic<bool> active_ = true;
  std::chrono::seconds timeout_in_seconds_;
};

#endif  // ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_
