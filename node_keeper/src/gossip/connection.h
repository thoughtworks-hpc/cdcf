/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_CONNECTION_H_
#define NODE_KEEPER_SRC_GOSSIP_CONNECTION_H_
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <asio.hpp>

#include "src/gossip/message.h"

namespace gossip {
class Connection : public std::enable_shared_from_this<Connection> {
 public:
  typedef std::function<void(asio::ip::tcp::socket *socket, const Address &,
                             const Message &)>
      ReceiveHandler;

  Connection(asio::ip::tcp::socket &&socket, ReceiveHandler on_receive)
      : socket_(std::move(socket)), on_receive_(on_receive) {}

  asio::ip::tcp::socket &Socket() { return socket_; }

  void Start() {
    auto buffer = std::make_shared<std::vector<uint8_t>>(1024, 0);
    auto that = shared_from_this();

    socket_.async_receive(asio::buffer(*buffer), [this, that, buffer](
                                                     const std::error_code
                                                         &error,
                                                     size_t bytes_transferred) {
      if (error) {
        return;
      }

      if (bytes_transferred != 0) {
        for (size_t decoded = 0; decoded < bytes_transferred;) {
          decoded +=
              message_.Decode(&(*buffer)[decoded], bytes_transferred - decoded);
          if (message_.IsSatisfied()) {
            auto remote = socket_.remote_endpoint();
            const Address address{remote.address().to_string(), remote.port()};
            on_receive_(&socket_, address, message_);
            message_.Reset();
          }
        }
      }
      Start();
    });
  }

 private:
  asio::ip::tcp::socket socket_;
  ReceiveHandler on_receive_;
  Message message_;
};
}  // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_CONNECTION_H_
