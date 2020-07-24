/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_CONFIG_H_
#define NODE_KEEPER_SRC_CONFIG_H_
#include <cdcf_config.h>

#include <sstream>
#include <string>
#include <vector>

#include "src/common.h"
#include "src/gossip.h"

namespace node_keeper {

class Config : public CDCFConfig {
 public:
  uint16_t port_ = 4748;
  std::string seeds_ = "";
  std::string app_;
  std::string app_args_;
  bool single_run_ = false;

  Config() {
    opt_group{custom_options_, "global"}
        .add(port_, "port,p", "set port")
        .add(seeds_, "seeds,s",
             "seeds of cluster, format in: `host:port,host:port`")
        .add(single_run_, "single_run", "run node keeper without actor system")
        .add(app_, "app", "set application path")
        .add(app_args_, "app-args", "set application arguments");
  }

  [[nodiscard]] std::vector<gossip::Address> GetSeeds() const {
    const auto addresses = split(seeds_, ',');
    std::vector<gossip::Address> result;
    for (auto& address_string : addresses) {
      auto words = split(address_string, ':');
      if (words.size() != 2) {
        continue;
      }
      auto& host = words[0];
      auto port = static_cast<uint16_t>(std::stoi(words[1]));
      result.push_back(gossip::Address{host, port});
    }
    return result;
  }
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_CONFIG_H_
