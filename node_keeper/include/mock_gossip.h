/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef CDCF_MOCK_GOSSIP_H
#define CDCF_MOCK_GOSSIP_H

#include <gmock/gmock.h>

#include <utility>
#include <vector>

#include "gossip.h"
#include "membership.h"

using namespace gossip;

namespace gossip {

bool operator==(const gossip::Payload &lhs, const gossip::Payload &rhs) {
  return lhs.data == rhs.data;
}
}  // namespace gossip

class MockTransport : public Transportable {
 public:
  MOCK_METHOD(ErrorCode, Gossip,
              (const std::vector<Address> &nodes, const Payload &data,
               DidGossipHandler didGossip),
              (override));

  MOCK_METHOD(ErrorCode, Push,
              (const Address &node, const void *data, size_t size,
               DidPushHandler didPush),
              (override));

  MOCK_METHOD(PullResult, Pull,
              (const Address &node, const void *data, size_t size,
               DidPullHandler didPull),
              (override));

  void RegisterGossipHandler(GossipHandler handler) override {
    gossip_handler_ = handler;
  }

  void RegisterPushHandler(PushHandler handler) override {
    push_handler_ = handler;
  }

  void RegisterPullHandler(PullHandler handler) override {
    pull_handler_ = handler;
  }

  void CallGossipHandler(const Address &address, const Payload &payload) {
    if (gossip_handler_ != nullptr) {
      gossip_handler_(address, payload);
    }
  }

  void CallPushHandler(const Address &address, const void *data, size_t size) {
    if (push_handler_ != nullptr) {
      push_handler_(address, data, size);
    }
  }

  void CallPullHandler(const Address &address, const void *data, size_t size) {
    if (pull_handler_ != nullptr) {
      pull_handler_(address, data, size);
    }
  }

 private:
  GossipHandler gossip_handler_;
  PushHandler push_handler_;
  PullHandler pull_handler_;
};

class MockSubscriber : public membership::Subscriber {
 public:
  MOCK_METHOD(void, Update, (), (override));
};

#endif  // CDCF_MOCK_GOSSIP_H