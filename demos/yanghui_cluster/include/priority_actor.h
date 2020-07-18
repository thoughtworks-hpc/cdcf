/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_PRIORITY_ACTOR_H
#define CDCF_PRIORITY_ACTOR_H

#include <caf/all.hpp>

class PriorityActor : public caf::event_based_actor {
 public:
  PriorityActor(caf::actor_config& cfg) : event_based_actor(cfg){};
  void enqueue(caf::mailbox_element_ptr ptr, caf::execution_unit* eu) override;

 private:
};

#endif  // CDCF_PRIORITY_ACTOR_H
