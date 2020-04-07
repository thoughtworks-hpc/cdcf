/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */

#ifndef CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_
#define CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_

#include <algorithm>

#include "caf/all.hpp"

class CDCFConfig : public caf::actor_system_config {
 public:
  CDCFConfig();
  CDCFConfig(CDCFConfig&& config) = default;
  CDCFConfig(const CDCFConfig& config) = delete;
  CDCFConfig& operator=(const CDCFConfig&) = delete;

  double threads_proportion = 1.0;
  enum class RetValue {
    kSuccess = 0,
    kFailure = 1,
  };

  RetValue parse_config(int argc, char** argv,
                        const char* ini_file_cstr = "cdcf-default.ini");
};

#endif  // CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_
