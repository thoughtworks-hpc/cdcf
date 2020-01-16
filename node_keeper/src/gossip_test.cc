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
#include <queue>

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
      auto &queue = receivedQueues_[i];
      peers_[i]->RegisterGossipHandler(
          [&](const Address &node, const Payload &data) {
            {
              std::lock_guard<std::mutex> lock(mutex);
              queue.emplace(data.data);
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
  std::array<std::queue<std::vector<uint8_t>>, 5> receivedQueues_;
  static constexpr const std::chrono::milliseconds kTimeout{
      std::chrono::milliseconds(1000)};
};

TEST_F(Gossip, ShouldReceiveGossipOnTheRightPeer) {
  const Payload sent("hello world!");

  peers_[0]->Gossip({addresses_[1]}, sent);

  auto &queue = receivedQueues_[1];
  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !queue.empty(); }));
  }
  EXPECT_THAT(queue.front(), Eq(sent.data));
}

TEST_F(Gossip, ShouldReceiveGossipOnAllRightPeers) {
  using gossip::Address, gossip::Payload;
  const Payload sent("hello world!");

  peers_[0]->Gossip({addresses_[1], addresses_[2]}, sent);

  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(cvs_[1].wait_for(
        lock, kTimeout, [&]() { return !receivedQueues_[1].empty(); }));
  }
  EXPECT_THAT(receivedQueues_[1].front(), Eq(sent.data));
  {
    std::unique_lock<std::mutex> lock(mutexes_[2]);
    ASSERT_TRUE(cvs_[2].wait_for(
        lock, kTimeout, [&]() { return !receivedQueues_[2].empty(); }));
  }
  EXPECT_THAT(receivedQueues_[2].front(), Eq(sent.data));
}

class Push : public Gossip {
 public:
  void SetUp() {
    Gossip::SetUp();
    for (size_t i = 0; i < peers_.size(); ++i) {
      auto &mutex = mutexes_[i];
      auto &cv = cvs_[i];
      auto &queue = receivedQueues_[i];
      peers_[i]->RegisterPushHandler(
          [&](const Address &node, const void *data, size_t size) {
            {
              std::lock_guard<std::mutex> lock(mutex);
              auto pointer = reinterpret_cast<const uint8_t *>(data);
              queue.emplace(pointer, pointer + size);
            }
            cv.notify_all();
          });
    }
  }
};

TEST_F(Push, ShouldPushDataToRemotePeer) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};

  auto result = peers_[0]->Push(addresses_[1], sent.data(), sent.size());

  ASSERT_THAT(result, Eq(gossip::ErrorCode::kOK));
  auto &queue = receivedQueues_[1];
  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !queue.empty(); }));
  }
  EXPECT_THAT(queue.front(), Eq(sent));
}

TEST_F(Push, ShouldPushTwoDataToRemotePeerSeparately) {
  const std::vector<uint8_t> sent(200, 12);
  peers_[0]->Push(addresses_[1], sent.data(), sent.size());

  auto result = peers_[0]->Push(addresses_[1], sent.data(), sent.size());

  ASSERT_THAT(result, Eq(gossip::ErrorCode::kOK));
  auto &queue = receivedQueues_[1];
  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return queue.size() == 2; }));
  }
  EXPECT_THAT(queue.front(), Eq(sent));
  EXPECT_THAT(queue.back(), Eq(sent));
}

TEST_F(Push, ShouldPushDataToRemotePeerAsynchronously) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};

  std::mutex mutex;
  std::condition_variable cv;
  bool done = false;
  auto result = peers_[0]->Push(addresses_[1], sent.data(), sent.size(),
                                [&](gossip::ErrorCode) {
                                  {
                                    std::lock_guard<std::mutex> lock(mutex);
                                    done = true;
                                  }
                                  cv.notify_all();
                                });

  ASSERT_THAT(result, Eq(gossip::ErrorCode::kOK));
  {
    std::unique_lock<std::mutex> lock(mutex);
    ASSERT_TRUE(cv.wait_for(lock, kTimeout, [&]() { return done; }));
  }
  auto &queue = receivedQueues_[1];
  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !queue.empty(); }));
  }
  EXPECT_THAT(queue.front(), Eq(sent));
}

TEST(Pull, ShouldPullDataFromRemotePeer) {
  gossip::Address addressA{"127.0.0.1", 5000};
  auto peerA = gossip::CreateTransport(addressA, addressA);
  gossip::Address addressB{"127.0.0.1", 5001};
  auto peerB = gossip::CreateTransport(addressB, addressB);
  const std::vector<uint8_t> pulled{5, 4, 3, 2, 1};
  peerB->RegisterPullHandler([&pulled](const Address &address, const void *data,
                                       size_t size) { return pulled; });

  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};
  auto result = peerA->Pull(addressB, sent.data(), sent.size());

  ASSERT_THAT(result.first, Eq(gossip::ErrorCode::kOK));
  ASSERT_THAT(result.second, Eq(pulled));
}
