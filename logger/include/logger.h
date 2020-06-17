/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef LOGGER_INCLUDE_LOGGER_H_
#define LOGGER_INCLUDE_LOGGER_H_

#include <memory>
#include <mutex>
#include <string>

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

  Logger(const std::string& module_name,
         const std::string& file_name = "default.log",
         int file_size_in_bytes = 0, int file_number = 0);

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

  template <typename... Args>
  void RawTrace(const char* file_name, int line_number, const std::string& fmt,
                const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->trace(filename_and_linenumber_format_ + fmt, args...,
                     fmt::arg(filename_arg_, file_name),
                     fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->trace(fmt, args...);
    }
  }

  template <typename... Args>
  void RawDebug(const char* file_name, int line_number, const std::string& fmt,
                const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->debug(filename_and_linenumber_format_ + fmt, args...,
                     fmt::arg(filename_arg_, file_name),
                     fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->debug(fmt, args...);
    }
  }

  template <typename... Args>
  void RawInfo(const char* file_name, int line_number, const std::string& fmt,
               const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->info(filename_and_linenumber_format_ + fmt, args...,
                    fmt::arg(filename_arg_, file_name),
                    fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->info(fmt, args...);
    }
  }

  template <typename... Args>
  void RawWarn(const char* file_name, int line_number, const std::string& fmt,
               const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->warn(filename_and_linenumber_format_ + fmt, args...,
                    fmt::arg(filename_arg_, file_name),
                    fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->warn(fmt, args...);
    }
  }

  template <typename... Args>
  void RawError(const char* file_name, int line_number, const std::string& fmt,
                const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->error(filename_and_linenumber_format_ + fmt, args...,
                     fmt::arg(filename_arg_, file_name),
                     fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->error(fmt, args...);
    }
  }

  template <typename... Args>
  void RawCritical(const char* file_name, int line_number,
                   const std::string& fmt, const Args&... args) {
    if (is_filename_and_linenumber_enabled_) {
      logger_->critical(filename_and_linenumber_format_ + fmt, args...,
                        fmt::arg(filename_arg_, file_name),
                        fmt::arg(linenumber_arg_, line_number));
    } else {
      logger_->critical(fmt, args...);
    }
  }

  void SetLevel(const std::string& level);
  void EnableFileNameAndLineNumber();
  void DisableFileNameAndLineNumber();

 private:
  bool is_filename_and_linenumber_enabled_ = true;
  const std::string filename_and_linenumber_format_ = "{file}:{line}: ";
  const std::string filename_arg_ = "file";
  const std::string linenumber_arg_ = "line";

 protected:
  Logger() = default;
  std::shared_ptr<spdlog::logger> logger_;
  static std::once_flag once_flag_;
};

class StdoutLogger : public Logger {
  static std::once_flag once_flag;

 public:
  explicit StdoutLogger(const std::string& module_name);
};

#define CDCF_LOGGER_TRACE(logger, ...) \
  logger->RawTrace(__FILE__, __LINE__, __VA_ARGS__)

#define CDCF_LOGGER_DEBUG(logger, ...) \
  logger->RawDebug(__FILE__, __LINE__, __VA_ARGS__)

#define CDCF_LOGGER_INFO(logger, ...) \
  logger->RawInfo(__FILE__, __LINE__, __VA_ARGS__)

#define CDCF_LOGGER_WARN(logger, ...) \
  logger->RawWarn(__FILE__, __LINE__, __VA_ARGS__)

#define CDCF_LOGGER_ERROR(logger, ...) \
  logger->RawError(__FILE__, __LINE__, __VA_ARGS__)

#define CDCF_LOGGER_CRITICAL(logger, ...) \
  logger->RawCritical(__FILE__, __LINE__, __VA_ARGS__)

}  // namespace cdcf

#endif  // LOGGER_INCLUDE_LOGGER_H_
