/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "../include/logger.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <cassert>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::mutex cdcf::Logger::mutex_;

cdcf::Logger::Logger(const std::string& module_name,
                     const std::string& file_name, int file_size_in_bytes,
                     int file_number) {
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    auto logger = spdlog::get(module_name);
    if (logger == nullptr) {
      if (file_size_in_bytes == 0) {
        logger_ = spdlog::basic_logger_mt(module_name, file_name);
      } else {
        logger_ = spdlog::rotating_logger_mt(module_name, file_name,
                                             file_size_in_bytes, file_number);
      }

      spdlog::flush_every(std::chrono::seconds(5));
    } else {
      logger_ = logger;
    }
    assert(logger_ != nullptr);
  }
}

void cdcf::Logger::SetLevel(const std::string& level) {
  spdlog::level::level_enum level_enum = spdlog::level::from_str(level);
  logger_->set_level(level_enum);
}

void cdcf::Logger::EnableFileNameAndLineNumber() {
  is_filename_and_linenumber_enabled_ = true;
}

void cdcf::Logger::DisableFileNameAndLineNumber() {
  is_filename_and_linenumber_enabled_ = false;
}

std::once_flag cdcf::StdoutLogger::once_flag;

cdcf::StdoutLogger::StdoutLogger(const std::string& module_name) {
  auto logger = spdlog::get(module_name);
  if (logger == nullptr) {
    std::call_once(once_flag, [this, &module_name]() {
      logger_ = spdlog::stdout_color_mt(module_name);
    });
  } else {
    logger_ = logger;
  }
}
