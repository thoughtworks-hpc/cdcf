/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/actor_union.h"

ActorUnion::ActorUnion(caf::actor_system& system,
                       caf::actor_pool::policy policy)
    : message_guarantor_(caf::actor(), system) {
  context = new caf::scoped_execution_unit(&system);
  pool_actor_ = caf::actor_pool::make(context, std::move(policy));
  message_guarantor_.SetReceiver(pool_actor_);
}

ActorUnion::~ActorUnion() { delete context; }

void ActorUnion::SendMsg(MessageID msg_id, const caf::message& msg) {
  message_guarantor_.SendMsg(msg_id, msg);
}

void ActorUnion::ConfirmMsg(MessageID msg_id) {
  message_guarantor_.ConfirmMsg(msg_id);
}

void ActorUnion::AddActor(caf::actor actor) {
  caf::anon_send(pool_actor_, caf::sys_atom::value, caf::put_atom::value,
                 actor);
}

void ActorUnion::RemoveActor(caf::actor actor) {
  caf::anon_send(pool_actor_, caf::sys_atom::value, caf::delete_atom::value,
                 actor);
}
