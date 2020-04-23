/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_
#define ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_

#include <utility>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

class ActorGuard {
 public:
  ActorGuard(caf::actor& keepActor,
             std::function<caf::actor(std::atomic<bool>&)> restart,
             caf::actor_system& system)
      : keep_actor_(keepActor),
        restart_fun_(std::move(restart)),
        sender_actor_(system) {}

  template <class... send, class return_function>
  bool SendAndReceive(return_function f,
                      std::function<void(caf::error)> err_deal,
                      const send&... xs) {
    if (active_) {
      caf::message send_message = caf::make_message(xs...);

      sender_actor_->request(keep_actor_, std::chrono::seconds(1), xs...)
          .receive(f, [&](caf::error err) {
            HandleSendFailed(send_message, f, err_deal, err);
          });
    }

    return active_;
  }

 private:
  template <class return_function>
  void HandleSendFailed(const caf::message& msg, return_function f,
                        std::function<void(caf::error)> err_deal,
                        caf::error err) {
    if ("system" != caf::to_string(err.category())) {
      // not system error, mean actor not down, this is a business error.
      err_deal(err);
      return;
    }

    std::cout << "send msg failed, try restart dest actor." << std::endl;
    keep_actor_ = restart_fun_(active_);

    if (active_) {
      std::cout << "restart actor success." << std::endl;
      (void)SendAndReceive(f, err_deal, msg);
    } else {
      std::cout << "restart actor failed. message:" << caf::to_string(msg)
                << " will not deliver." << std::endl;
    }
  }

  caf::actor keep_actor_;
  caf::scoped_actor sender_actor_;
  std::function<caf::actor(std::atomic<bool>&)> restart_fun_;
  std::atomic<bool> active_ = true;
};

#endif  // ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_GUARD_H_
