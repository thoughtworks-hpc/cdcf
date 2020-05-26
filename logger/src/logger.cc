/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "../include/logger.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "spdlog/sinks/rotating_file_sink.h"

cdcf::Logger::Logger(const std::string& module_name,
                     const std::string& file_name, int file_size,
                     int file_number) {
  if (file_size == 0) {
    logger_ = spdlog::basic_logger_mt(module_name, file_name);
  } else {
    logger_ = spdlog::rotating_logger_mt(module_name, file_name, file_size,
                                         file_number);
  }
}

void cdcf::Logger::SetLevel(cdcf::Logger::log_level level) {
  switch (level) {
    case LOG_LEVEL_TRACE:
      logger_->set_level(spdlog::level::trace);
      break;
    case LOG_LEVEL_DEBUG:
      logger_->set_level(spdlog::level::debug);
      break;
    case LOG_LEVEL_INFO:
      logger_->set_level(spdlog::level::info);
      break;
    case LOG_LEVEL_WARN:
      logger_->set_level(spdlog::level::warn);
      break;
    case LOG_LEVEL_ERROR:
      logger_->set_level(spdlog::level::err);
      break;
    case LOG_LEVEL_CRITICAL:
      logger_->set_level(spdlog::level::critical);
      break;
    case LOG_LEVEL_OFF:
      logger_->set_level(spdlog::level::off);
      break;
  }
}

void cdcf::Logger::EnableFileNameAndLineNumber() {
  is_filename_and_linenumber_enabled_ = true;
}

void cdcf::Logger::DisableFileNameAndLineNumber() {
  is_filename_and_linenumber_enabled_ = false;
}
