/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_FAKE_GOSSIP_H_
#define NODE_KEEPER_SRC_FAKE_GOSSIP_H_

#include <memory>
#include <vector>

using gossip::Address;
using gossip::ErrorCode;

class UnreachableTransport : public gossip::Transport {
 public:
  UnreachableTransport(const Address &udp, const Address &tcp)
      : Transport(udp, tcp), self_address_(tcp) {}

  PullResult Pull(const Address &node, const void *data, size_t size,
                  DidPullHandler did_pull) override {
    bool is_to_make_unreachable = false;

    for (const auto &address : unreachable_addresses_) {
      if (address == node) {
        is_to_make_unreachable = true;
      }
    }

    if (is_to_make_unreachable) {
      PullResult result{ErrorCode::kUnknown, std::vector<uint8_t>()};
      pull_future_ = std::async(std::launch::async, ([did_pull, result]() {
                                  did_pull(result);
                                  return result;
                                }));
      return result;
    } else {
      return Transport::Pull(node, data, size, did_pull);
    }
  }

  void MakeUnreachableTo(const Address &peer) {
    unreachable_addresses_.push_back(peer);
  }

 private:
  std::vector<Address> unreachable_addresses_;
  std::future<PullResult> pull_future_;
  Address self_address_;
};

std::unique_ptr<UnreachableTransport> CreateUnreachableTransport(
    const Address &udp, const Address &tcp) {
  return std::make_unique<UnreachableTransport>(udp, tcp);
}

#endif  // NODE_KEEPER_SRC_FAKE_GOSSIP_H_
