/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <iostream>
// anti clang-format sort
#include <asio.hpp>

namespace gossip {

using asio::ip::udp;
using std::chrono::milliseconds;

class Transport : public Transportable {
 public:
  Transport(const Address &udp, const Address &tcp) try
      : udpSocket_(ioContext_, udp::endpoint(udp::v4(), udp.port)),
        stopReceiving_(false) {
    StartReceiveGossip();
    ioThread_ = std::thread(&Transport::IORoutine, this);
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

  virtual ~Transport() {
    stopReceiving_ = true;
    ioContext_.stop();
    if (ioThread_.joinable()) {
      ioThread_.join();
    }
  }

 public:
  virtual int Gossip(const std::vector<Address> &nodes,
                     const Payload &payload) {
    udp::resolver resolver(ioContext_);
    for (const auto &node : nodes) {
      auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
      const auto &endpoint = *endpoints.begin();
      auto sent = udpSocket_.send_to(asio::buffer(payload.data), endpoint);
      assert(sent == payload.data.size() && "all bytes should be sent");
    }
    return 0;
  }

  virtual void RegisterGossipHandler(GossipHandler handler) {
    gossipHandler_ = handler;
  }

  virtual ErrorCode Push(const Address &node, const void *data, size_t size,
                         milliseconds timeout, DidPushHandler didPush) {
    return ErrorCode::kOK;
  }
  virtual void RegisterPushHandler(PushHandler handler) {}
  virtual PullResult Pull(const Address &node, const void *data, size_t size,
                          milliseconds timeout, DidPullHandler didPull) {
    return PullResult{ErrorCode::kOK, {0}};
  }
  virtual void RegisterPullHandler(PullHandler handler) {}

 private:
  void StartReceiveGossip() {
    auto buffer =
        std::make_shared<std::vector<uint8_t>>(Payload::kMaxPayloadSize);
    auto remote = std::make_shared<udp::endpoint>();
    udpSocket_.async_receive_from(
        asio::buffer(*buffer), *remote,
        [this, buffer, remote](const asio::error_code &error,
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

 private:
  asio::io_context ioContext_;
  udp::socket udpSocket_;
  bool stopReceiving_;
  std::thread ioThread_;
  GossipHandler gossipHandler_;
};

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
