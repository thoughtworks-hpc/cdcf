/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <future>
#include <optional>
// anti clang-format sort
#include <asio.hpp>

#include "src/gossip/connection.h"
#include "src/gossip/message.h"

namespace gossip {

using asio::ip::tcp, asio::ip::udp;

class Transport : public Transportable {
 public:
  Transport(const Address &udp, const Address &tcp) try
      : udpSocket_(ioContext_, udp::endpoint(udp::v4(), udp.port)),
        acceptor_(ioContext_, tcp::endpoint(tcp::v4(), tcp.port)) {
    StartReceiveGossip();
    StartAccept();
    ioThread_ = std::thread(&Transport::IORoutine, this);
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

  virtual ~Transport() {
    ioContext_.stop();
    if (ioThread_.joinable()) {
      ioThread_.join();
    }
  }

 public:
  virtual ErrorCode Gossip(const std::vector<Address> &nodes,
                           const Payload &payload, DidGossipHandler didGossip) {
    udp::resolver resolver(ioContext_);
    for (const auto &node : nodes) {
      auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
      const auto &endpoint = *endpoints.begin();
      if (didGossip) {
        udpSocket_.async_send_to(
            asio::buffer(payload.data), endpoint,
            [didGossip](std::error_code error, std::size_t) {
              didGossip(error ? ErrorCode::kUnknown : ErrorCode::kOK);
            });
      } else {
        auto sent = udpSocket_.send_to(asio::buffer(payload.data), endpoint);
        assert(sent == payload.data.size() && "all bytes should be sent");
      }
    }
    return ErrorCode::kOK;
  }

  virtual void RegisterGossipHandler(GossipHandler handler) {
    gossipHandler_ = handler;
  }

  virtual ErrorCode Push(const Address &node, const void *data, size_t size,
                         DidPushHandler didPush) {
    tcp::resolver resolver(ioContext_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    tcp::socket socket(ioContext_);
    asio::connect(socket, endpoints);
    auto buffer = Message(Message::Type::kPush, data, size).Encode();
    if (didPush) {
      socket.async_write_some(asio::buffer(buffer),
                              [didPush](const std::error_code &error, size_t) {
                                didPush(ErrorCode::kOK);
                              });
      return ErrorCode::kOK;
    } else {
      auto sent = socket.write_some(asio::buffer(buffer));
      return sent == buffer.size() ? ErrorCode::kOK : ErrorCode::kUnknown;
    }
  }

  virtual void RegisterPushHandler(PushHandler handler) {
    pushHandler_ = handler;
  }

  virtual PullResult Pull(const Address &node, const void *data, size_t size,
                          DidPullHandler didPull) try {
    PullResult result{ErrorCode::kUnknown, {}};
    tcp::resolver resolver(ioContext_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    auto socket = std::make_shared<tcp::socket>(ioContext_);
    asio::connect(*socket, endpoints);
    auto out = Message(Message::Type::kPull, data, size).Encode();
    if (didPull) {
      auto onDidPull = [this, didPull, result, socket](
                           const std::error_code &err, size_t) mutable {
        if (err) {
          didPull(result);
          return;
        }
        if (auto message = ReadMessage(&*socket)) {
          result = PullResult{ErrorCode::kOK, message->Data()};
        }
        didPull(result);
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
  } catch (const std::exception &) {
    PullResult result{ErrorCode::kUnknown, {}};
    if (didPull) {
      std::async(std::launch::async, didPull, result);
    }
    return result;
  }

  virtual void RegisterPullHandler(PullHandler handler) {
    pullHandler_ = handler;
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
    udpSocket_.async_receive_from(
        asio::buffer(*buffer), *remote,
        [this, buffer, remote](const std::error_code &error,
                               std::size_t bytes_transferred) {
          if (error) {
            return;
          }
          if (gossipHandler_) {
            const Address address{remote->address().to_string(),
                                  remote->port()};
            gossipHandler_(address, Payload(buffer->data(), bytes_transferred));
          }
          StartReceiveGossip();
        });
  }

  void IORoutine() {
    try {
      ioContext_.run();
    } catch (...) {
    }
  }

  void StartAccept() {
    auto onReceive = [this](tcp::socket *socket, const Address &address,
                            const Message &message) {
      const auto &buffer = message.Data();
      if (message.Type() == Message::Type::kPush) {
        pushHandler_(address, buffer.data(), buffer.size());
      } else if (message.Type() == Message::Type::kPull) {
        OnPull(socket, address, buffer);
      }
    };
    acceptor_.async_accept(
        [this, onReceive](const std::error_code &error, tcp::socket socket) {
          if (!error) {
            std::make_shared<Connection>(std::move(socket), onReceive)->Start();
          }
          StartAccept();
        });
  }

 private:
  void OnPull(tcp::socket *socket, const Address &address,
              const std::vector<uint8_t> &request) {
    auto response = pullHandler_(address, request.data(), request.size());
    Message respondMessage(Message::Type::kPullResponse, response.data(),
                           response.size());
    auto out = respondMessage.Encode();
    socket->write_some(asio::buffer(out));
  }

  asio::io_context ioContext_;
  udp::socket udpSocket_;
  std::thread ioThread_;
  GossipHandler gossipHandler_;
  tcp::acceptor acceptor_;
  PushHandler pushHandler_;
  PullHandler pullHandler_;
};  // namespace gossip

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
