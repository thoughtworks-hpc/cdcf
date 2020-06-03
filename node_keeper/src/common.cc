/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/common.h"

#include <sstream>

std::vector<std::string> node_keeper::split(const std::string& input,
                                            char delim) {
  std::vector<std::string> result;
  std::stringstream ss(input);
  std::string token;
  while (std::getline(ss, token, delim)) {
    result.push_back(token);
  }
  return result;
}
