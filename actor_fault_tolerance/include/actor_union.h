/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_ACTOR_UNION_H
#define CDCF_ACTOR_UNION_H
#include "./actor_guard.h"

class ActorUnion {
 public:
  ActorUnion(caf::actor_system& system, caf::actor_pool::policy policy);
  virtual ~ActorUnion();
  void SendMsg(MessageID msg_id, const caf::message& msg);
  void ConfirmMsg(MessageID msg_id);
  void AddActor(caf::actor actor);
  void RemoveActor(caf::actor actor);

  template <class... Ts>
  void Send(MessageID msg_id, const Ts&... xs) {
    message_guarantor_.Send(msg_id, xs...);
  }

 private:
  caf::actor pool_actor_;
  ActorMessageGuarantor message_guarantor_;
  caf::scoped_execution_unit* context;
};

#endif  // CDCF_ACTOR_UNION_H
