/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./logger.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iostream>

std::vector<spdlog::sink_ptr> GenerateSinks(const CDCFConfig& config);

TEST(GenerateSinksTest, ShouldSinksHaveOnlyConsoleSinkGivenDefaultConfig) {
  CDCFConfig config;
  auto sinks = GenerateSinks(config);

  EXPECT_EQ(1, sinks.size());
}

TEST(GenerateSinksTest, ShouldSinksHaveNoConsoleSinkGivenLogToConfigIsFalse) {
  CDCFConfig config;
  config.no_log_to_console_ = true;

  auto sinks = GenerateSinks(config);

  EXPECT_EQ(0, sinks.size());
}

TEST(GenerateSinksTest, ShouldSinksHaveFileSinkGivenLogFileNotEmpty) {
  CDCFConfig config;
  config.no_log_to_console_ = true;
  config.log_file_ = "cdcf.log";

  auto sinks = GenerateSinks(config);

  EXPECT_EQ(1, sinks.size());
}

TEST(GenerateSinksTest,
     ShouldSinksHaveRotatingSinkGivenLogFileSizeInBytesNot0) {
  CDCFConfig config;
  config.no_log_to_console_ = true;
  config.log_file_ = "cdcf.log";
  config.log_file_size_in_bytes_ = 1024;

  auto sinks = GenerateSinks(config);

  EXPECT_EQ(1, sinks.size());
}

TEST(GenerateSinksTest, ShouldSinksHaveConsoleAndFileSinkGivenCorrectConfig) {
  CDCFConfig config;
  config.log_file_ = "cdcf.log";

  auto sinks = GenerateSinks(config);

  EXPECT_EQ(2, sinks.size());
}

TEST(GenerateSinksTest,
     ShouldSinksHaveConsoleAndRotatingSinkGivenCorrectConfig) {
  CDCFConfig config;
  config.log_file_ = "cdcf.log";
  config.log_file_size_in_bytes_ = 1024;

  auto sinks = GenerateSinks(config);

  EXPECT_EQ(2, sinks.size());
}

TEST(LoggerTest, ShouldLogToConsoleCorrectlyGivenDefaultConfig) {
  testing::internal::CaptureStdout();
  CDCFConfig config;
  cdcf::Logger::Init(config);

  CDCF_LOGGER_DEBUG("Debug Log");
  CDCF_LOGGER_INFO("Hello World");

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, testing::HasSubstr("Hello World"));
  EXPECT_THAT(output, testing::HasSubstr("info"));
  EXPECT_THAT(output, testing::HasSubstr("logger_test.cc:"));
  EXPECT_TRUE(output.find("Debug Log") == std::string::npos);
}

static std::string ReadFileContent(const std::string& file) {
  std::stringstream ss;
  std::ifstream ifs;
  ifs.open(file);
  std::string buffer;
  while (ifs) {
    getline(ifs, buffer);
    ss << buffer;
  }
  ifs.close();
  return ss.str();
}

TEST(LoggerTest, ShouldLogToFileCorrectlyGivenLogFileName) {
  CDCFConfig config;
  config.log_file_ = "logger_test.log";
  std::remove(config.log_file_.c_str());
  cdcf::Logger::Init(config);

  CDCF_LOGGER_DEBUG("Debug Log");
  CDCF_LOGGER_INFO("Hello World");
  cdcf::Logger::Flush();

  std::string output = ReadFileContent(config.log_file_);
  EXPECT_THAT(output, testing::HasSubstr("Hello World"));
  EXPECT_THAT(output, testing::HasSubstr("info"));
  EXPECT_THAT(output, testing::HasSubstr("logger_test.cc:"));
  EXPECT_TRUE(output.find("Debug Log") == std::string::npos);
}

TEST(LoggerTest, ShouldNotLogFilenameAndLineNumGivenCorrectConfig) {
  testing::internal::CaptureStdout();
  CDCFConfig config;
  config.log_no_display_filename_and_line_number_ = true;
  cdcf::Logger::Init(config);

  CDCF_LOGGER_INFO("Hello World");

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(output.find("logger_test.cc") == std::string::npos);
}
