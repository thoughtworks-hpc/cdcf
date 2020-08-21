/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef DEMOS_LOAD_BALANCER_SERVER_H_
#define DEMOS_LOAD_BALANCER_SERVER_H_
#include <cdcf/actor_system.h>

#include <memory>
#include <string>
#include <utility>

#include <asio.hpp>

#include "./router.h"
#include "./session.h"

class Server {
 public:
  Server(asio::io_context& io_context, caf::actor_system& system,
         const std::string& host, uint16_t actor_system_port,
         uint16_t reception_port)
      : acceptor_(io_context,
                  asio::ip::tcp::endpoint(asio::ip::tcp::v4(), reception_port)),
        router_(system, host, actor_system_port) {
    DoAccept();
  }

 private:
  void DoAccept() {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
          if (!ec) {
            std::make_shared<Session>(std::move(socket), router_)->Start();
          }
          DoAccept();
        });
  }

  WorkerRouter router_;
  asio::ip::tcp::acceptor acceptor_;
};
#endif  // DEMOS_LOAD_BALANCER_SERVER_H_
