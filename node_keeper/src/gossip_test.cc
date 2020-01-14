/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

using testing::NotNull, testing::Eq;

TEST(Transport, ShouldReturnTransportIfCreateTransportWithAvailablePort) {
  gossip::Address udp = {"127.0.0.1", 5000};
  gossip::Address tcp = {"127.0.0.1", 5000};

  auto transport = gossip::CreateTransport(udp, tcp);

  EXPECT_THAT(transport, NotNull());
}

TEST(Transport, ShouldThrowErrorIfCreateTransportWithOccupiedPort) {
  gossip::Address udp = {"127.0.0.1", 5000};
  gossip::Address tcp = {"127.0.0.1", 5000};
  auto occupiedTransport = gossip::CreateTransport(udp, tcp);

  EXPECT_THROW(gossip::CreateTransport(udp, tcp), gossip::PortOccupied);
}

using gossip::Address, gossip::Payload;

class Gossip : public testing::Test {
 public:
  void SetUp() {
    std::generate(addresses_.begin(), addresses_.end(), [i = 0]() mutable {
      ++i;
      return gossip::Address{"127.0.0.1", (uint16_t)(5000 + i)};
    });
    std::transform(addresses_.begin(), addresses_.end(), peers_.begin(),
                   [](const gossip::Address &address) {
                     return gossip::CreateTransport(address, address);
                   });
    for (size_t i = 0; i < peers_.size(); ++i) {
      auto &mutex = mutexes_[i];
      auto &cv = cvs_[i];
      auto &received = receives_[i];
      peers_[i]->RegisterGossipHandler(
          [&](const Address &node, const Payload &data) {
            {
              std::lock_guard<std::mutex> lock(mutex);
              received = std::make_unique<std::vector<uint8_t>>(data.data);
            }
            cv.notify_all();
          });
    }
  }

 protected:
  std::array<gossip::Address, 5> addresses_;
  std::array<std::unique_ptr<gossip::Transportable>, 5> peers_;
  std::array<std::mutex, 5> mutexes_;
  std::array<std::condition_variable, 5> cvs_;
  std::array<std::unique_ptr<std::vector<uint8_t>>, 5> receives_;
  static constexpr const std::chrono::milliseconds kTimeout{
      std::chrono::milliseconds(1000)};
};

TEST_F(Gossip, ShouldReceiveGossipOnTheRightPeer) {
  const Payload sent("hello world!");
  std::unique_ptr<std::vector<uint8_t>> received;

  peers_[0]->Gossip({addresses_[1]}, sent);

  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !!receives_[1]; }));
  }
  EXPECT_THAT(*receives_[1], Eq(sent.data));
}

TEST_F(Gossip, ShouldReceiveGossipOnAllRightPeers) {
  using gossip::Address, gossip::Payload;
  const Payload sent("hello world!");

  peers_[0]->Gossip({addresses_[1], addresses_[2]}, sent);

  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !!receives_[1]; }));
  }
  EXPECT_THAT(*receives_[1], Eq(sent.data));
  {
    std::unique_lock<std::mutex> lock(mutexes_[2]);
    ASSERT_TRUE(
        cvs_[2].wait_for(lock, kTimeout, [&]() { return !!receives_[2]; }));
  }
  EXPECT_THAT(*receives_[2], Eq(sent.data));
}
