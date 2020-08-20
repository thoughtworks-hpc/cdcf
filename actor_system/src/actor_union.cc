/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "cdcf/actor_union.h"

namespace cdcf {

ActorUnion::ActorUnion(caf::actor_system& system,
                       caf::actor_pool::policy policy,
                       std::chrono::seconds timeout_in_seconds)
    : sender_actor_(system), timeout_in_seconds_(timeout_in_seconds) {
  context_ = new caf::scoped_execution_unit(&system);
  pool_actor_ = caf::actor_pool::make(context_, std::move(policy));
}

ActorUnion::~ActorUnion() { delete context_; }

void ActorUnion::AddActor(const caf::actor& actor) {
  caf::anon_send(pool_actor_, caf::sys_atom::value, caf::put_atom::value,
                 actor);
  actor_count_++;
}

void ActorUnion::RemoveActor(const caf::actor& actor) {
  caf::anon_send(pool_actor_, caf::sys_atom::value, caf::delete_atom::value,
                 actor);
  actor_count_--;
}

caf::error make_error(actor_union_error x) {
  return {static_cast<uint8_t>(x), caf::atom("act_union")};
}

std::string to_string(actor_union_error x) {
  switch (x) {
    case actor_union_error::all_actor_out_of_work:
      return "all actor out of work";
    default:
      return "-unknown-error-";
  }
}

}  // namespace cdcf