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
  std::string name_ = "node";
  uint16_t port_ = 4748;
  std::string host_ = "localhost";
  std::string seeds_ = "";
  std::string log_file_ = "node_keeper.log";
  std::string log_level_ = "info";
  uint16_t log_file_size_in_bytes_ = 0;
  uint16_t log_file_number_ = 0;

  Config() {
    opt_group{custom_options_, "global"}
        .add(name_, "name,n", "set node name")
        .add(port_, "port,p", "set port")
        .add(host_, "host,H", "set host")
        .add(seeds_, "seeds,s",
             "seeds of cluster, format in: `host:port,host:port`")
        .add(log_file_, "log-file",
             "set log file name, default: node_keeper.log,"
             " format in: /user/cdcf/node_keeper.log")
        .add(log_level_, "log-level",
             "set log level, default: info, format in: info")
        .add(log_file_size_in_bytes_, "log-file-size",
             "set maximum rotating log file size in bytes,"
             " default: unlimited, format in: 1024")
        .add(log_file_number_, "log-file-num",
             "set maximum rotating log file number, each file has size"
             " 'log-file-size', default: 0, format in: 1024");
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
