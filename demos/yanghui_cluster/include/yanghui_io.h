/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_IO_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_IO_H_
#include <caf/openssl/all.hpp>

#include "../yanghui_config.h"
#include "../../../logger/include/logger.h"

class YanghuiIO {
  const bool use_ssl_;
  caf::actor_system& system_;

 public:
  YanghuiIO(const config& cfg, caf::actor_system& system)
      : use_ssl_(!cfg.openssl_cafile.empty() ||
                     !cfg.openssl_certificate.empty() ||
                     !cfg.openssl_key.empty()),
        system_(system) {
    CDCF_LOGGER_INFO("enable ssl: {}", use_ssl_);
  }

  template <class Handle>
  caf::expected<uint16_t> publish(const Handle& whom, uint16_t port,
                                  const char* in = nullptr,
                                  bool reuse = false) {
    return use_ssl_ ? caf::openssl::publish(whom, port, in, reuse)
                        : caf::io::publish(whom, port, in, reuse);
  }

  template <class ActorHandle = caf::actor>
  caf::expected<ActorHandle> remote_actor(caf::actor_system& sys,
                                          std::string host, uint16_t port) {
    return use_ssl_ ? caf::openssl::remote_actor(system_, host, port)
                        : caf::io::remote_actor(system_, host, port);
  }
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_IO_H_
