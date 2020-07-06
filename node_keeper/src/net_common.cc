/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/net_common.h"

#include <asio.hpp>

#include "src/membership.h"

int membership::ResolveHostName(const std::string& address,
                                std::string& hostname,
                                std::string& ip_address) {
  asio::io_context io_context;
  using asio::ip::tcp;
  tcp::resolver resolver(io_context);
  asio::error_code ec;
  tcp::resolver::results_type endpoints = resolver.resolve(address, "", ec);
  if (ec) {
    return MEMBERSHIP_CONFIG_IP_ADDRESS_INVALID;
  }

  bool entry_found = false;
  decltype(endpoints.begin()) found_entry_iter;
  for (auto entry_iter = endpoints.begin(); entry_iter != endpoints.end();
       ++entry_iter) {
    if (entry_iter->endpoint().address().is_v4()) {
      entry_found = true;
      found_entry_iter = entry_iter;
    }
  }

  if (entry_found) {
    ip_address = found_entry_iter->endpoint().address().to_string();
    hostname = found_entry_iter->host_name();
  } else {
    return MEMBERSHIP_CONFIG_IP_ADDRESS_INVALID;
  }

  return MEMBERSHIP_SUCCESS;
}
