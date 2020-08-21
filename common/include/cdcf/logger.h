/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef COMMON_INCLUDE_CDCF_LOGGER_H_
#define COMMON_INCLUDE_CDCF_LOGGER_H_

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>

#include "cdcf/cdcf_config.h"

namespace cdcf {

class Logger {
 public:
  static void Init(const CDCFConfig& config);
  static void Flush();

 private:
  static std::shared_ptr<spdlog::logger> logger_;
};

#define CDCF_LOGGER_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)

#define CDCF_LOGGER_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#define CDCF_LOGGER_INFO(...) SPDLOG_INFO(__VA_ARGS__)

#define CDCF_LOGGER_WARN(...) SPDLOG_WARN(__VA_ARGS__)

#define CDCF_LOGGER_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)

#define CDCF_LOGGER_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

}  // namespace cdcf

#endif  // COMMON_INCLUDE_CDCF_LOGGER_H_
