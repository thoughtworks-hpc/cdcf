/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_ACTOR_H_
#define NODE_KEEPER_SRC_ACTOR_H_

#include <string>

namespace node_keeper {
struct Actor {
  std::string address;

  bool operator<(const Actor& rhs) const;
};
}  // namespace node_keeper

#endif  // NODE_KEEPER_SRC_ACTOR_H_
