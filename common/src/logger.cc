/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "cdcf/logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

std::shared_ptr<spdlog::logger> cdcf::Logger::logger_;

std::vector<spdlog::sink_ptr> GenerateSinks(const cdcf::CDCFConfig& config) {
  std::vector<spdlog::sink_ptr> sinks;
  if (!config.no_log_to_console_) {
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  }

  if (!config.log_file_.empty()) {
    if (config.log_file_size_in_bytes_ == 0) {
      sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
          config.log_file_));
    } else {
      sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          config.log_file_, config.log_file_size_in_bytes_,
          config.log_file_number_));
    }
  }
  return sinks;
}

void cdcf::Logger::Init(const cdcf::CDCFConfig& config) {
  if (logger_) {
    CDCF_LOGGER_CRITICAL("Logger init many times.");
  }

  auto sinks = GenerateSinks(config);
  logger_ = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(),
                                             sinks.end());

  std::string log_pattern;
  if (!config.log_no_display_filename_and_line_number_) {
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
                    !config.log_no_display_filename_and_line_number_);
}

void cdcf::Logger::Flush() {
  if (logger_) {
    logger_->flush();
  }
}
