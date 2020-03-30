/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_ACTOR_MONITOR_CONFIG_H
#define CDCF_ACTOR_MONITOR_CONFIG_H

#include "../../config_manager/include/cdcf_config.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

class config : public cdcf_config {
 public:
  std::string host = "localhost";
  uint16_t port = 56089;
  std::string test_type = "";
  config() {
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node")
        .add(test_type, "test, T", "set test type: down exit error");
  }
};

#endif  // CDCF_ACTOR_MONITOR_CONFIG_H
