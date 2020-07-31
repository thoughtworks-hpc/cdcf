/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "caf/openssl/all.hpp"

caf::behavior make_pong_behavior() {
  return {[](std::string str) {
    std::cout << "receive msg: " << str << std::endl;
  }};
}

caf::behavior make_pong_ssl_behavior() {
  return {[](std::string str) {
    std::cout << "ssl receive msg: " << str << std::endl;
  }};
}

void caf_main(caf::actor_system& system, const caf::actor_system_config& cfg) {
  std::cout << "cafile: " << cfg.openssl_cafile << std::endl;
  std::cout << "certificate: " << cfg.openssl_certificate << std::endl;
  std::cout << "key: " << cfg.openssl_key << std::endl;

  caf::expected<uint16_t> expected_port(0);
  auto spong = system.spawn(make_pong_behavior);
  std::cout << "spong id: " << spong.id()
            << ", address: " << caf::to_string(spong.address()) << std::endl;
  expected_port = caf::io::publish(spong, 50602);
  if (!expected_port) {
    std::cout << "publish spong failed. reason: "
              << system.render(expected_port.error()) << std::endl;
    exit(1);
  }
  auto spong_ssl = system.spawn(make_pong_ssl_behavior);
  std::cout << "spong_ssl id: " << spong_ssl.id()
            << ", address: " << caf::to_string(spong_ssl.address())
            << std::endl;
  expected_port = caf::openssl::publish(spong_ssl, 50603);
  if (!expected_port) {
    std::cout << "publish spong failed. reason: "
              << system.render(expected_port.error()) << std::endl;
    exit(1);
  }

  std::cout << "already publish all actor, press [Enter] to exit" << std::endl;

  std::string dummy;
  std::getline(std::cin, dummy);
}

CAF_MAIN(caf::io::middleman, caf::openssl::manager)
// Run configuration
// program args: --openssl.cafile=ca-noenc.crt
// --openssl.certificate=node1.cert.pem --openssl.key=node1.key.pem working
// directory: /path/to/docker/ssl
