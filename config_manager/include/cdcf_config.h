/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */

#ifndef CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_
#define CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_

#include <algorithm>
#include <string>
#include <vector>

#include "caf/all.hpp"

class CDCFConfig : public caf::actor_system_config {
 public:
  CDCFConfig();
  CDCFConfig(CDCFConfig&& config) = default;
  CDCFConfig(const CDCFConfig& config) = delete;
  CDCFConfig& operator=(const CDCFConfig&) = delete;

  double threads_proportion = 1.0;
  std::string log_file_ = "cdcf.log";
  std::string log_level_ = "info";
  uint16_t log_file_size_in_bytes_ = 0;
  uint16_t log_file_number_ = 0;
  bool log_display_filename_and_line_number_ = true;

  enum class RetValue {
    kSuccess = 0,
    kFailure = 1,
  };

  RetValue parse_config(int argc, char** argv,
                        const char* ini_file_cstr = "cdcf-default.ini");
  RetValue parse_config(const std::vector<std::string>& args,
                        const char* ini_file_cstr = "cdcf-default.ini");
};

#endif  // CONFIG_MANAGER_INCLUDE_CDCF_CONFIG_H_
