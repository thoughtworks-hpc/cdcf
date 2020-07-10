/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "../include/logger.h"

#include <spdlog/sinks/basic_file_sink.h>

#include "spdlog/sinks/rotating_file_sink.h"

std::shared_ptr<spdlog::logger> cdcf::Logger::logger_;

std::once_flag cdcf::Logger::once_flag_;

void cdcf::Logger::Init(const CDCFConfig& config) {
  std::call_once(once_flag_, [&config]() {
    if (config.log_file_size_in_bytes_ == 0) {
      logger_ = spdlog::basic_logger_mt("basic_logger", config.log_file_);
    } else {
      logger_ = spdlog::rotating_logger_mt("rotating_logger", config.log_file_,
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
    logger_->set_pattern(log_pattern);
    logger_->set_level(spdlog::level::from_str(config.log_level_));
    spdlog::set_default_logger(logger_);
    spdlog::flush_every(std::chrono::seconds(5));

    CDCF_LOGGER_DEBUG("Log file name is {}", config.log_file_);
    CDCF_LOGGER_DEBUG("Log file level is {}", config.log_level_);
    CDCF_LOGGER_DEBUG("Log file size is {}", config.log_file_size_in_bytes_);
    CDCF_LOGGER_DEBUG("Log file number is {}", config.log_file_number_);
    CDCF_LOGGER_DEBUG("Log file display filename and line number is {}",
                      config.log_display_filename_and_line_number_);
  });
}
