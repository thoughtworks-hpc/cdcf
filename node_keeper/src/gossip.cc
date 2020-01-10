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
      : udpSocket_(ioContext_, udp::endpoint(udp::v4(), udp.port)) {
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

 public:
  virtual int Gossip(const std::vector<Address> &nodes, const Payload &data) {
    return 0;
  }
  virtual void RegisterGossipHandler(GossipHandler handler) {}
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
  asio::io_context ioContext_;
  udp::socket udpSocket_;
};

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
