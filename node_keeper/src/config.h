/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_CONFIG_H_
#define NODE_KEEPER_SRC_CONFIG_H_
#include <cdcf_config.h>

#include <sstream>
#include <string>
#include <vector>

#include "include/gossip.h"

namespace node_keeper {
std::vector<std::string> split(const std::string& input, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(input);
  std::string token;
  while (std::getline(ss, token, delim)) {
    result.push_back(token);
  }
  return result;
}

class Config : public cdcf_config {
 public:
  std::string name_ = "node";
  uint16_t port_ = 4748;
  std::string host_ = "localhost";
  std::string seeds_ = "";

  Config() {
    opt_group{custom_options_, "global"}
        .add(name_, "name,n", "set node name")
        .add(port_, "port,p", "set port")
        .add(host_, "host,H", "set host")
        .add(seeds_, "seeds,s",
             "seeds of cluster, format in: `host:port,host:port`");
  }

  std::vector<gossip::Address> GetSeeds() const {
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
