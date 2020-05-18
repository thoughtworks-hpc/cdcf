/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef CDCF_LOGGER_H
#define CDCF_LOGGER_H

#include <memory>

#include "spdlog/spdlog.h"

namespace cdcf {

class Logger {
 public:
  enum log_level {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_CRITICAL = 5,
    LOG_LEVEL_OFF = 6,
  };

  Logger(const std::string& module_name, const std::string& file_name);

  template <typename... Args>
  void Trace(const std::string& fmt, const Args&... args) {
    logger_->trace(fmt, args...);
  }

  template <typename... Args>
  void Debug(const std::string& fmt, const Args&... args) {
    logger_->debug(fmt, args...);
  }

  template <typename... Args>
  void Info(const std::string& fmt, const Args&... args) {
    logger_->info(fmt, args...);
  }

  template <typename... Args>
  void Warn(const std::string& fmt, const Args&... args) {
    logger_->warn(fmt, args...);
  }

  template <typename... Args>
  void Error(const std::string& fmt, const Args&... args) {
    logger_->error(fmt, args...);
  }

  template <typename... Args>
  void Critical(const std::string& fmt, const Args&... args) {
    logger_->critical(fmt, args...);
  }

  void SetLevel(enum log_level level);

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace cdcf

#endif  // CDCF_LOGGER_H
