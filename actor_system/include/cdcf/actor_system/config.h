/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_CDCF_ACTOR_SYSTEM_CONFIG_H_
#define ACTOR_SYSTEM_INCLUDE_CDCF_ACTOR_SYSTEM_CONFIG_H_
#include <cdcf/cdcf_config.h>

#include <cstdint>
#include <string>

namespace cdcf::actor_system {
class Config : public cdcf::CDCFConfig {
 public:
  uint16_t port_ = 4750;

  Config() {
    opt_group{custom_options_, "global"}.add(port_, "actor_system_port,a",
                                             "set port for actor_system");
  }
};
}  // namespace cdcf::actor_system
#endif  // ACTOR_SYSTEM_INCLUDE_CDCF_ACTOR_SYSTEM_CONFIG_H_
