/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_NODE_KEEPER_H_
#define NODE_KEEPER_SRC_NODE_KEEPER_H_
#include <string>
#include <vector>

#include "../include/membership.h"

namespace node_keeper {

class NodeKeeper {
  membership::Membership membership_;

 public:
  NodeKeeper(const std::string& name, const gossip::Address& address,
             const std::vector<gossip::Address>& seeds = {});

  std::vector<membership::Member> GetMembers() const {
    return membership_.GetMembers();
  }
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_NODE_KEEPER_H_
