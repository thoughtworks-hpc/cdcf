/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "actor.h"
bool node_keeper::Actor::operator<(const node_keeper::Actor& rhs) const {
  return address < rhs.address;
}
