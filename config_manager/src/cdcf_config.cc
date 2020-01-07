#include "cdcf_config.h"

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

  return SUCCESS;
}
