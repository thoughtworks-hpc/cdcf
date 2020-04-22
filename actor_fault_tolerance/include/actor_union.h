/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_ACTOR_UNION_H
#define CDCF_ACTOR_UNION_H
#include "actor_guard.h"

enum class actor_union_error : uint8_t { all_actor_out_of_work = 1 };

caf::error make_error(actor_union_error x);
std::string to_string(actor_union_error x);

class ActorUnion {
 public:
  ActorUnion(caf::actor_system& system, caf::actor_pool::policy policy);
  virtual ~ActorUnion();
  void AddActor(const caf::actor& actor);
  void RemoveActor(caf::actor actor);

  template <class... send, class return_function>
  void SendAndReceiveWithTryTime(return_function f,
                                 std::function<void(caf::error)> err_deal,
                                 uint16_t has_try_time, const send&... xs) {
    caf::message send_message = caf::make_message(xs...);
    sender_actor_->request(pool_actor_, std::chrono::seconds(1), xs...)
        .receive(f, [=](caf::error err) {
          HandleSendFailed(send_message, f, err_deal, err, has_try_time);
        });
  }

  template <class... send, class return_function>
  void SendAndReceive(return_function f,
                      std::function<void(caf::error)> err_deal,
                      const send&... xs) {
    SendAndReceiveWithTryTime(f, err_deal, 0, xs...);
  }

 private:
  caf::actor pool_actor_;
  caf::scoped_execution_unit* context_;
  caf::scoped_actor sender_actor_;
  uint16_t actor_count_ = 0;

  template <class return_function>
  void HandleSendFailed(const caf::message& msg, return_function f,
                        std::function<void(caf::error)> err_deal,
                        caf::error err, uint16_t has_try_time) {
    if ("system" != caf::to_string(err.category())) {
      // not system error, mean actor not down, this is a business error.
      err_deal(err);
      return;
    }

    has_try_time++;

    if (has_try_time <= actor_count_) {
      std::cout << "send msg failed, try send to another actor. try_time:"
                << has_try_time << std::endl;
      SendAndReceiveWithTryTime(f, err_deal, has_try_time, msg);
    } else {
      std::cout << "send msg failed, return error. try_time:" << has_try_time
                << std::endl;

      caf::error ret_error =
          make_error(actor_union_error::all_actor_out_of_work);
      err_deal(ret_error);
    };
  }
};

#endif  // CDCF_ACTOR_UNION_H
