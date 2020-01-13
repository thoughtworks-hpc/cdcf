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
        stopReceiving_(false),
        gossipThread_(&Transport::ReceiveRoutine, this) {
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

  virtual ~Transport() {
    stopReceiving_ = true;
    // intended to close socket first to wakeup gossipThread to response to join
    udpSocket_.close();
    if (gossipThread_.joinable()) {
      gossipThread_.join();
    }
  }

 public:
  virtual int Gossip(const std::vector<Address> &nodes,
                     const Payload &payload) {
    udp::resolver resolver(ioContext_);
    auto endpoints =
        resolver.resolve(nodes[0].host, std::to_string(nodes[0].port));
    const auto &endpoint = *endpoints.begin();
    auto sent = udpSocket_.send_to(asio::buffer(payload.data), endpoint);
    assert(sent == payload.data.size() && "all bytes should be sent");
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
  void ReceiveRoutine() {
    while (!stopReceiving_) {
      try {
        std::vector<unsigned char> buffer(Payload::kMaxPayloadSize);
        udp::endpoint remote;
        size_t len = udpSocket_.receive_from(asio::buffer(buffer), remote);
        if (gossipHandler_) {
          const Address address{remote.address().to_string(), remote.port()};
          gossipHandler_(address, Payload(buffer.data(), len));
        }
      } catch (...) {
      }
    }
  }

 private:
  asio::io_context ioContext_;
  udp::socket udpSocket_;
  bool stopReceiving_;
  std::thread gossipThread_;
  GossipHandler gossipHandler_;
};

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
