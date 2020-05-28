/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CONFIG_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CONFIG_H_
#include <cdcf_config.h>

#include <cstdint>
#include <string>

namespace actor_system {
class Config : public CDCFConfig {
 public:
  std::string name_ = "";
  std::string host_ = "localhost";
  uint16_t port_ = 4750;

  Config() {
    opt_group{custom_options_, "global"}
        .add(name_, "name,n", "set node name")
        .add(host_, "host,H", "set host")
        .add(port_, "actor_system_port,a", "set port for actor_system");
  }
};
}  // namespace actor_system
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_CONFIG_H_
