/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include "../include/cdcf_config.h"

cdcf_config::cdcf_config() {
  opt_group{custom_options_, "global"}.add(
      threads_proportion, "threads_proportion", "set threads proportion");
}
cdcf_config::RET_VALUE cdcf_config::parse_config(int argc, char** argv,
                                                 const char* ini_file_cstr) {
  if (auto err = parse(argc, argv, ini_file_cstr)) {
    std::cerr << "error while parsing CLI and file options: "
              << caf::actor_system_config::render(err) << std::endl;
    return FAILURE;
  }

  if (cli_helptext_printed) {
    return FAILURE;
  }

  size_t thread_num =
      floor(std::thread::hardware_concurrency() * threads_proportion);
  set("scheduler.max-threads", std::max(thread_num, 4ul));

  return SUCCESS;
}
