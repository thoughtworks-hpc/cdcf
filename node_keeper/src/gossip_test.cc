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

using gossip::Address, gossip::CreateTransport, gossip::ErrorCode,
    gossip::Payload, gossip::PortOccupied, gossip::Transportable;
using testing::NotNull, testing::Eq;

TEST(Transport, ShouldReturnTransportIfCreateTransportWithAvailablePort) {
  Address udp = {"127.0.0.1", 5000};
  Address tcp = {"127.0.0.1", 5000};

  auto transport = CreateTransport(udp, tcp);

  EXPECT_THAT(transport, NotNull());
}

TEST(Transport, ShouldThrowErrorIfCreateTransportWithOccupiedPort) {
  Address udp = {"127.0.0.1", 5000};
  Address tcp = {"127.0.0.1", 5000};
  auto occupiedTransport = CreateTransport(udp, tcp);

  EXPECT_THROW(CreateTransport(udp, tcp), PortOccupied);
}

class Gossip : public testing::Test {
 public:
  void SetUp() {
    std::generate(addresses_.begin(), addresses_.end(), [i = 0]() mutable {
      ++i;
      return Address{"127.0.0.1", (uint16_t)(5000 + i)};
    });
    std::transform(addresses_.begin(), addresses_.end(), peers_.begin(),
                   [](const Address &address) {
                     return CreateTransport(address, address);
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
  std::array<Address, 5> addresses_;
  std::array<std::unique_ptr<Transportable>, 5> peers_;
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
    local_ = CreateTransport(addressLocal_, addressLocal_);
    remote_ = CreateTransport(addressRemote_, addressRemote_);
    remote_->RegisterPushHandler(
        [&](const Address &node, const void *data, size_t size) {
          {
            std::lock_guard<std::mutex> lock(mutex_);
            auto pointer = reinterpret_cast<const uint8_t *>(data);
            queue_.emplace(pointer, pointer + size);
          }
          cv_.notify_all();
        });
  }

 protected:
  Address addressLocal_{"127.0.0.1", 5000};
  Address addressRemote_{"127.0.0.1", 5001};
  std::unique_ptr<Transportable> local_;
  std::unique_ptr<Transportable> remote_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::vector<uint8_t>> queue_;
  static constexpr const std::chrono::milliseconds kTimeout{
      std::chrono::milliseconds(1000)};
};

TEST_F(Push, ShouldPushDataToRemotePeer) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};

  auto result = local_->Push(addressRemote_, sent.data(), sent.size());

  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  {
    std::unique_lock<std::mutex> lock(mutex_);
    ASSERT_TRUE(
        cv_.wait_for(lock, kTimeout, [&]() { return !queue_.empty(); }));
  }
  EXPECT_THAT(queue_.front(), Eq(sent));
}

TEST_F(Push, ShouldPushTwoDataToRemotePeerSeparately) {
  const std::vector<uint8_t> sent(200, 12);
  local_->Push(addressRemote_, sent.data(), sent.size());

  auto result = local_->Push(addressRemote_, sent.data(), sent.size());

  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  {
    std::unique_lock<std::mutex> lock(mutex_);
    ASSERT_TRUE(
        cv_.wait_for(lock, kTimeout, [&]() { return queue_.size() == 2; }));
  }
  EXPECT_THAT(queue_.front(), Eq(sent));
  EXPECT_THAT(queue_.back(), Eq(sent));
}

TEST_F(Push, ShouldPushDataToRemotePeerAsynchronously) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};

  std::mutex mutex;
  std::condition_variable cv;
  bool done = false;
  auto didPush = [&](ErrorCode) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    cv.notify_all();
  };
  auto result = local_->Push(addressRemote_, sent.data(), sent.size(), didPush);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    ASSERT_TRUE(
        cv_.wait_for(lock, kTimeout, [&]() { return !queue_.empty(); }));
  }
  EXPECT_THAT(queue_.front(), Eq(sent));
  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  {
    std::unique_lock<std::mutex> lock(mutex);
    ASSERT_TRUE(cv.wait_for(lock, kTimeout, [&]() { return done; }));
  }
}

TEST(Pull, ShouldPullDataFromRemotePeer) {
  Address addressA{"127.0.0.1", 5000};
  auto peerA = CreateTransport(addressA, addressA);
  Address addressB{"127.0.0.1", 5001};
  auto peerB = CreateTransport(addressB, addressB);
  const std::vector<uint8_t> pulled{5, 4, 3, 2, 1};
  peerB->RegisterPullHandler([&pulled](const Address &address, const void *data,
                                       size_t size) { return pulled; });

  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};
  auto result = peerA->Pull(addressB, sent.data(), sent.size());

  ASSERT_THAT(result.first, Eq(ErrorCode::kOK));
  ASSERT_THAT(result.second, Eq(pulled));
}

TEST(Pull, ShouldPullDataFromRemotePeerAsynchronously) {
  Address addressA{"127.0.0.1", 5000};
  auto peerA = CreateTransport(addressA, addressA);
  Address addressB{"127.0.0.1", 5001};
  auto peerB = CreateTransport(addressB, addressB);
  const std::vector<uint8_t> pulled{5, 4, 3, 2, 1};
  peerB->RegisterPullHandler([&pulled](const Address &address, const void *data,
                                       size_t size) { return pulled; });

  std::mutex mutex;
  std::condition_variable cv;
  using gossip::Pullable;
  std::unique_ptr<Pullable::PullResult> response;
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};
  auto didPull = [&](const Pullable::PullResult &result) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      response = std::make_unique<Pullable::PullResult>(result);
    }
    cv.notify_all();
  };
  auto result = peerA->Pull(addressB, sent.data(), sent.size(), didPull);

  std::unique_lock<std::mutex> lock(mutex);
  auto timeout = std::chrono::milliseconds(5000);
  ASSERT_TRUE(cv.wait_for(lock, timeout, [&]() { return !!response; }));
  ASSERT_THAT(response->first, Eq(ErrorCode::kOK));
  ASSERT_THAT(response->second, Eq(pulled));
}
