/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */

#ifndef COMMON_INCLUDE_CDCF_CDCF_CONFIG_H_
#define COMMON_INCLUDE_CDCF_CDCF_CONFIG_H_

#include <algorithm>
#include <string>
#include <vector>

#include <caf/all.hpp>

namespace cdcf {
class CDCFConfig : public caf::actor_system_config {
 public:
  CDCFConfig();
  CDCFConfig(CDCFConfig&& config) = default;
  CDCFConfig(const CDCFConfig& config) = delete;
  CDCFConfig& operator=(const CDCFConfig&) = delete;

  double threads_proportion = 1.0;
  std::string role_ = "";
  std::string log_file_ = "";
  std::string log_level_ = "info";
  bool no_log_to_console_ = false;
  uint16_t log_file_size_in_bytes_ = 0;
  uint16_t log_file_number_ = 0;
  bool log_no_display_filename_and_line_number_ = false;
  std::string name_ = "node";
  std::string host_ = "localhost";

  enum class RetValue {
    kSuccess = 0,
    kFailure = 1,
  };

  RetValue parse_config(int argc, char** argv,
                        const char* ini_file_cstr = "cdcf-default.ini");
  RetValue parse_config(const std::vector<std::string>& args,
                        const char* ini_file_cstr = "cdcf-default.ini");
};
}  // namespace cdcf

#endif  // COMMON_INCLUDE_CDCF_CDCF_CONFIG_H_
