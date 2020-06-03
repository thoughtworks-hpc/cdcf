/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_NODE_KEEPER_H_
#define NODE_KEEPER_SRC_NODE_KEEPER_H_
#include <memory>
#include <string>
#include <vector>

#include "../../logger/include/logger.h"
#include "src/config.h"
#include "src/membership.h"

namespace node_keeper {

class NodeKeeper {
  membership::Membership membership_;
  std::shared_ptr<cdcf::Logger> logger_;
  const std::string logger_name_ = "node_keeper";

 public:
  NodeKeeper(const std::string& name, const gossip::Address& address,
             const std::vector<gossip::Address>& seeds = {},
             const Config& other_config = {});

  std::vector<membership::Member> GetMembers() const {
    return membership_.GetMembers();
  }

  void Run();
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_NODE_KEEPER_H_
