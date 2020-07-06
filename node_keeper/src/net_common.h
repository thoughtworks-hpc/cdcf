/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_NET_COMMON_H_
#define NODE_KEEPER_SRC_NET_COMMON_H_

#include <string>

namespace membership {

int ResolveHostName(const std::string& address, std::string& hostname,
                    std::string& ip_address);

}

#endif  // NODE_KEEPER_SRC_NET_COMMON_H_
