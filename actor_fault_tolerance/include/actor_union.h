/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_UNION_H_
#define ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_UNION_H_
#include <string>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

enum class actor_union_error : uint8_t { all_actor_out_of_work = 1 };

caf::error make_error(actor_union_error x);
std::string to_string(actor_union_error x);

class ActorUnion {
 public:
  ActorUnion(caf::actor_system& system, caf::actor_pool::policy policy);
  virtual ~ActorUnion();
  void AddActor(const caf::actor& actor);
  void RemoveActor(const caf::actor& actor);

  template <caf::message_priority P = caf::message_priority::normal,
            class... send_type, class return_function_type>
  void SendAndReceive(return_function_type return_function,
                      std::function<void(caf::error)> err_deal,
                      const send_type&... messages) {
    SendAndReceiveWithTryTime(return_function, err_deal, 0, messages...);
  }

 private:
  caf::actor pool_actor_;
  caf::scoped_execution_unit* context_;
  caf::scoped_actor sender_actor_;
  uint16_t actor_count_ = 0;

  template <caf::message_priority P = caf::message_priority::normal,
            class... send_type, class return_function_type>
  void SendAndReceiveWithTryTime(
      return_function_type return_function,
      std::function<void(caf::error)> handle_error_function,
      uint16_t has_try_time, const send_type&... messages) {
    caf::message send_message = caf::make_message(messages...);
    sender_actor_->request<P>(pool_actor_, std::chrono::seconds(1), messages...)
        .receive(return_function, [=](caf::error err) {
          HandleSendFailed(send_message, return_function, handle_error_function,
                           err, has_try_time);
        });
  }

  template <class return_function_type>
  void HandleSendFailed(const caf::message& msg,
                        return_function_type return_function,
                        std::function<void(caf::error)> handle_error_function,
                        caf::error err, uint16_t has_try_time) {
    if ("system" != caf::to_string(err.category())) {
      // not system error, mean actor not down, this is a business error.
      handle_error_function(err);
      return;
    }

    has_try_time++;

    if (has_try_time <= actor_count_) {
      std::cout << "send msg failed, try send to another actor. try_time:"
                << has_try_time << std::endl;
      SendAndReceiveWithTryTime(return_function, handle_error_function,
                                has_try_time, msg);
    } else {
      std::cout << "send msg failed, return error. try_time:" << has_try_time
                << std::endl;
      caf::error ret_error =
          make_error(actor_union_error::all_actor_out_of_work);
      handle_error_function(ret_error);
    }
  }
};

#endif  // ACTOR_FAULT_TOLERANCE_INCLUDE_ACTOR_UNION_H_
