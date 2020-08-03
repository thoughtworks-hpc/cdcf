/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

/**
 * Wrong example, don't mix `caf.io` and `caf.openssl.`
 */

#include <iostream>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "caf/openssl/all.hpp"

caf::actor connect_tcp_worker(caf::actor_system &system,
                              const std::string &host) {
  auto result = caf::io::remote_actor(system, host, 50602);
  if (!result) {
    std::cout << "connect failed, reason: " << system.render(result.error())
              << std::endl;
    exit(1);
  }
  std::cout << "address: " << caf::to_string(result->address()) << std::endl;
  return *result;
}

caf::actor connect_ssl_worker(caf::actor_system &system,
                              const std::string &host) {
  auto result_ssl = caf::openssl::remote_actor(system, host, 50603);
  if (!result_ssl) {
    std::cout << "ssl connect failed, reason: "
              << system.render(result_ssl.error()) << std::endl;
    exit(1);
  }
  std::cout << "address: " << caf::to_string(result_ssl->address())
            << std::endl;
  return *result_ssl;
}

[[noreturn]] void caf_main(caf::actor_system &system) {
  std::string dummy;
  //    std::cout << "Please input remote host: ";
  //    std::getline(std::cin, dummy);
  //    std::string host = dummy;
  std::string host = "localhost";
  auto spong = connect_tcp_worker(system, host);
  auto spong_ssl = connect_ssl_worker(system, host);

  while (true) {
    std::getline(std::cin, dummy);
    if (dummy == "n") {
      caf::anon_send(spong, "hello");
      continue;
    }
    if (dummy == "s") {
      caf::anon_send(spong_ssl, "hello");
      continue;
    }
  }
}

CAF_MAIN(caf::io::middleman, caf::openssl::manager)
// Run configuration
// program args: --openssl.cafile=ca-noenc.crt
// --openssl.certificate=node2.cert.pem --openssl.key=node2.key.pem working
// directory: /path/to/docker/ssl
