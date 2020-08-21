/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_ACTOR_MONITOR_ACTOR_MONITOR_CONFIG_H_
#define DEMOS_ACTOR_MONITOR_ACTOR_MONITOR_CONFIG_H_

#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "cdcf/cdcf_config.h"

class config : public cdcf::CDCFConfig {
 public:
  std::string host = "localhost";
  uint16_t port = 56088;
  std::string test_type = "";
  config() {
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node")
        .add(test_type, "test, T", "set test type: down exit error");
  }
};

#endif  // DEMOS_ACTOR_MONITOR_ACTOR_MONITOR_CONFIG_H_
