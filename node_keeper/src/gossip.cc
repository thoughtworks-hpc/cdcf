/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

namespace gossip {

using std::chrono::milliseconds;

class Transport : public Transportable {
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
};

std::unique_ptr<Transportable> CreateTransport(const Address &upd,
                                               const Address &tcp) {
  return std::make_unique<Transport>();
}

}  // namespace gossip
