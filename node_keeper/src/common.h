/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_COMMON_H_
#define NODE_KEEPER_SRC_COMMON_H_

#include <string>
#include <vector>

namespace node_keeper {

std::vector<std::string> split(const std::string& input, char delim);

}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_COMMON_H_
