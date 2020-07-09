/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "../include/logger.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <cassert>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::mutex cdcf::Logger::mutex_;
std::shared_ptr<spdlog::logger> cdcf::Logger::g_logger_;

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
void cdcf::Logger::Init(const CDCFConfig& config) {
  // Todo(Yujia.Li): 加一个std::call_once()
  if (config.log_file_size_in_bytes_ == 0) {
    g_logger_ = spdlog::basic_logger_mt("basic_logger", config.log_file_);
  } else {
    g_logger_ = spdlog::rotating_logger_mt("rotating_logger", config.log_file_,
                                           config.log_file_size_in_bytes_,
                                           config.log_file_number_);
  }
  std::string log_pattern;
  if (config.log_display_filename_and_line_number_) {
    //    [2020-07-09 16:51:29.684] [info]
    //    [/Users/xxx/github_repo/cdcf/logger/src/logger_test.cc:11] hello
    //    word, 1

    log_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%g:%#] %v";
  } else {
    //    [2020-07-09 16:51:50.236] [info] hello word, 1
    log_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
  }
  g_logger_->set_pattern(log_pattern);
  g_logger_->set_level(spdlog::level::from_str(config.log_level_));
  spdlog::set_default_logger(g_logger_);
  spdlog::flush_every(std::chrono::seconds(5));
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
