/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <future>
#include <optional>

#include <asio.hpp>

#include "src/gossip/connection.h"
#include "src/gossip/message.h"

namespace gossip {

using asio::ip::tcp, asio::ip::udp;

class Transport : public Transportable {
 public:
  Transport(const Address &udp, const Address &tcp) try
      : upd_socket_(io_context_, udp::endpoint(udp::v4(), udp.port)),
        acceptor_(io_context_, tcp::endpoint(tcp::v4(), tcp.port)) {
    StartReceiveGossip();
    StartAccept();
    io_thread_ = std::thread(&Transport::IORoutine, this);
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

  virtual ~Transport() {
    io_context_.stop();
    if (io_thread_.joinable()) {
      io_thread_.join();
    }
  }

 public:
  virtual ErrorCode Gossip(const std::vector<Address> &nodes,
                           const Payload &payload,
                           DidGossipHandler did_gossip) try {
    udp::resolver resolver(io_context_);
    for (const auto &node : nodes) {
      auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
      const auto &endpoint = *endpoints.begin();
      if (did_gossip) {
        upd_socket_.async_send_to(
            asio::buffer(payload.data), endpoint,
            [did_gossip](std::error_code error, std::size_t) {
              did_gossip(error ? ErrorCode::kUnknown : ErrorCode::kOK);
            });
      } else {
        auto sent = upd_socket_.send_to(asio::buffer(payload.data), endpoint);
        assert(sent == payload.data.size() && "all bytes should be sent");
      }
    }
    return ErrorCode::kOK;
  } catch (const asio::system_error &e) {
    auto error = ExtractError(e);
    if (did_gossip) {
      did_gossip(error);
      return ErrorCode::kOK;
    } else {
      return error;
    }
  }

  virtual void RegisterGossipHandler(GossipHandler handler) {
    gossip_handler_ = handler;
  }

  virtual ErrorCode Push(const Address &node, const void *data, size_t size,
                         DidPushHandler did_push) try {
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    tcp::socket socket(io_context_);
    asio::connect(socket, endpoints);
    auto buffer = Message(Message::Type::kPush, data, size).Encode();
    if (did_push) {
      socket.async_write_some(asio::buffer(buffer),
                              [did_push](const std::error_code &error, size_t) {
                                did_push(ErrorCode::kOK);
                              });
      return ErrorCode::kOK;
    } else {
      auto sent = socket.write_some(asio::buffer(buffer));
      return sent == buffer.size() ? ErrorCode::kOK : ErrorCode::kUnknown;
    }
  } catch (const asio::system_error &e) {
    auto result = ExtractError(e);
    if (did_push) {
      asio::post(io_context_, std::bind(did_push, result));
      return ErrorCode::kOK;
    } else {
      return result;
    }
  }

  virtual void RegisterPushHandler(PushHandler handler) {
    push_handler_ = handler;
  }

  virtual PullResult Pull(const Address &node, const void *data, size_t size,
                          DidPullHandler did_pull) try {
    PullResult result{ErrorCode::kUnknown, {}};
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    auto socket = std::make_shared<tcp::socket>(io_context_);
    asio::connect(*socket, endpoints);
    auto out = Message(Message::Type::kPull, data, size).Encode();
    if (did_pull) {
      auto onDidPull = [this, did_pull, result, socket](
                           const std::error_code &err, size_t) mutable {
        if (err) {
          did_pull(result);
          return;
        }
        if (auto message = ReadMessage(&*socket)) {
          result = PullResult{ErrorCode::kOK, message->Data()};
        }
        did_pull(result);
      };
      socket->async_write_some(asio::buffer(out), onDidPull);
    } else {
      auto sent = socket->write_some(asio::buffer(out));
      if (sent != out.size()) {
        return result;
      }
      auto message = ReadMessage(&*socket);
      return message ? PullResult{ErrorCode::kOK, message->Data()} : result;
    }
    return result;
  } catch (const asio::system_error &e) {
    PullResult result{ExtractError(e), {}};
    if (did_pull) {
      asio::post(io_context_, std::bind(did_pull, result));
      return {ErrorCode::kOK, {}};
    }
    return result;
  }

  virtual void RegisterPullHandler(PullHandler handler) {
    pull_handler_ = handler;
  }

 private:
  std::optional<Message> ReadMessage(tcp::socket *socket) {
    Message message;
    auto buffer = std::vector<uint8_t>(1024, 0);
    while (true) {
      auto bytes_transferred = socket->read_some(asio::buffer(buffer, 1024));
      if (bytes_transferred == 0) {
        return std::nullopt;
      }
      for (size_t decoded = 0; decoded < bytes_transferred;) {
        decoded +=
            message.Decode(&buffer[decoded], bytes_transferred - decoded);
        if (message.IsSatisfied()) {
          return std::optional<Message>(std::move(message));
        }
      }
    }
  }

  void StartReceiveGossip() {
    auto buffer =
        std::make_shared<std::vector<uint8_t>>(Payload::kMaxPayloadSize);
    auto remote = std::make_shared<udp::endpoint>();
    upd_socket_.async_receive_from(
        asio::buffer(*buffer), *remote,
        [this, buffer, remote](const std::error_code &error,
                               std::size_t bytes_transferred) {
          if (error) {
            return;
          }
          if (gossip_handler_) {
            const Address address{remote->address().to_string(),
                                  remote->port()};
            gossip_handler_(address,
                            Payload(buffer->data(), bytes_transferred));
          }
          StartReceiveGossip();
        });
  }

  void IORoutine() {
    try {
      io_context_.run();
    } catch (...) {
    }
  }

  void StartAccept() {
    auto on_receive = [this](tcp::socket *socket, const Address &address,
                             const Message &message) {
      const auto &buffer = message.Data();
      if (message.Type() == Message::Type::kPush) {
        push_handler_(address, buffer.data(), buffer.size());
      } else if (message.Type() == Message::Type::kPull) {
        OnPull(socket, address, buffer);
      }
    };
    acceptor_.async_accept([this, on_receive](const std::error_code &error,
                                              tcp::socket socket) {
      if (!error) {
        std::make_shared<Connection>(std::move(socket), on_receive)->Start();
      }
      StartAccept();
    });
  }

 private:
  void OnPull(tcp::socket *socket, const Address &address,
              const std::vector<uint8_t> &request) {
    auto response = pull_handler_(address, request.data(), request.size());
    Message respondMessage(Message::Type::kPullResponse, response.data(),
                           response.size());
    auto out = respondMessage.Encode();
    socket->write_some(asio::buffer(out));
  }

  ErrorCode ExtractError(const asio::system_error &e) {
    switch (e.code().value()) {
      case asio::error::netdb_errors::host_not_found:
      case asio::error::netdb_errors::host_not_found_try_again:
        return ErrorCode::kHostNotFound;
      default:
        return ErrorCode::kUnknown;
    }
  }

  asio::io_context io_context_;
  udp::socket upd_socket_;
  std::thread io_thread_;
  GossipHandler gossip_handler_;
  tcp::acceptor acceptor_;
  PushHandler push_handler_;
  PullHandler pull_handler_;
};  // namespace gossip

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
