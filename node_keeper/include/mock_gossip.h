//
// Created by Zhao Wenbo on 2020/1/15.
//

#ifndef CDCF_MOCK_GOSSIP_H
#define CDCF_MOCK_GOSSIP_H

#include <gmock/gmock.h>

#include <vector>

#include "gossip.h"

using namespace gossip;

// class Gossipable {
// public:
//  // payload dissemination via gossip protocol
//  virtual int Gossip(const std::vector<Address> &nodes,
//                     const Payload &data) = 0;
//
//  typedef std::function<void(const Address &node, const Payload &data)>
//      GossipHandler;
//  virtual void RegisterGossipHandler(GossipHandler handler) = 0;
//};

namespace gossip {

bool operator==(const gossip::Payload &lhs, const gossip::Payload &rhs) {
  return lhs.data == rhs.data;
}
}  // namespace gossip

class MockTransport : public Transportable {
  //  MOCK_METHOD(void, PenUp, (), (override));
  //  MOCK_METHOD(void, PenDown, (), (override));
  //  MOCK_METHOD(void, Forward, (int distance), (override));
  //  MOCK_METHOD(void, Turn, (int degrees), (override));
 public:
  MOCK_METHOD(ErrorCode, Gossip,
              (const std::vector<Address> &nodes, const Payload &data,
               DidGossipHandler didGossip),
              (override));

  // MOCK_METHOD(void, RegisterGossipHandler, (GossipHandler handler),
  // (override));
  void RegisterGossipHandler(GossipHandler handler) override {
    handler_ = handler;
  }

  void CallGossipHandler(const Address &address, const Payload &payload) {
    handler_(address, payload);
  }

 private:
  GossipHandler handler_;

  //  void SetGossipMsg(const Address &address, )
};

#endif  // CDCF_MOCK_GOSSIP_H