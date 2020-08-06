/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include "../include/cdcf_config.h"

CDCFConfig::CDCFConfig() {
  opt_group{custom_options_, "global"}
      .add(threads_proportion, "threads_proportion", "set threads proportion")
      .add(log_file_, "log_file",
           "set log file name,"
           " format in: /user/cdcf/cdcf.log")
      .add(log_level_, "log_level",
           "set log level, default: info, format in: info")
      .add(no_log_to_console_, "no_log_to_console",
           "set whether to print log to console, default: true")
      .add(log_file_size_in_bytes_, "log_file_size",
           "set maximum rotating log file size in bytes,"
           " default: unlimited, format in: 1024")
      .add(log_file_number_, "log_file_num",
           "set maximum rotating log file number, each file has size"
           " 'log_file_size', default: 0, format in: 1024")
      .add(log_display_filename_and_line_number_,
           "log_display_filename_and_line_num",
           "enable display filename and line num in log")
      .add(role_, "role", "set node role, default: ''")
      .add(name_, "name,n", "set node name")
      .add(host_, "host,H", "set host");
}

CDCFConfig::RetValue CDCFConfig::parse_config(int argc, char** argv,
                                              const char* ini_file_cstr) {
  if (auto err = parse(argc, argv, ini_file_cstr)) {
    std::cerr << "error while parsing CLI and file options: "
              << caf::actor_system_config::render(err) << std::endl;
    return RetValue::kFailure;
  }

  if (cli_helptext_printed) {
    return RetValue::kFailure;
  }

  size_t thread_num = static_cast<size_t>(
      floor(std::thread::hardware_concurrency() * threads_proportion));
  const size_t min_thread_num = 4;
  set("scheduler.max-threads", std::max(thread_num, min_thread_num));

  return RetValue::kSuccess;
}

CDCFConfig::RetValue CDCFConfig::parse_config(
    const std::vector<std::string>& args, const char* ini_file_cstr) {
  if (auto err = parse(args, ini_file_cstr)) {
    std::cerr << "error while parsing CLI and file options: "
              << caf::actor_system_config::render(err) << std::endl;
    return RetValue::kFailure;
  }

  if (cli_helptext_printed) {
    return RetValue::kFailure;
  }

  size_t thread_num = static_cast<size_t>(
      floor(std::thread::hardware_concurrency() * threads_proportion));

  const size_t min_thread_size = 4;
  set("scheduler.max-threads", std::max(thread_num, min_thread_size));

  return RetValue::kSuccess;
}
