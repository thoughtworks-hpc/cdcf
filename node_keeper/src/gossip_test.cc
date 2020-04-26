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
#include <future>
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
      auto &queue = received_queues_[i];
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
  std::array<std::queue<std::vector<uint8_t>>, 5> received_queues_;
  static constexpr const std::chrono::milliseconds kTimeout{
      std::chrono::milliseconds(1000)};
};

TEST_F(Gossip, ShouldReceiveGossipOnTheRightPeer) {
  const Payload sent("hello world!");

  peers_[0]->Gossip({addresses_[1]}, sent);

  auto &queue = received_queues_[1];
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
        lock, kTimeout, [&]() { return !received_queues_[1].empty(); }));
  }
  EXPECT_THAT(received_queues_[1].front(), Eq(sent.data));
  {
    std::unique_lock<std::mutex> lock(mutexes_[2]);
    ASSERT_TRUE(cvs_[2].wait_for(
        lock, kTimeout, [&]() { return !received_queues_[2].empty(); }));
  }
  EXPECT_THAT(received_queues_[2].front(), Eq(sent.data));
}

TEST_F(Gossip, ShouldReceiveGossipWhenWasGossipAsynchronously) {
  const Payload sent("hello world!");

  std::promise<void> promise;
  peers_[0]->Gossip({addresses_[1]}, sent,
                    [&](ErrorCode error) { promise.set_value(); });

  auto future = promise.get_future();
  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
  auto &queue = received_queues_[1];
  {
    std::unique_lock<std::mutex> lock(mutexes_[1]);
    ASSERT_TRUE(
        cvs_[1].wait_for(lock, kTimeout, [&]() { return !queue.empty(); }));
  }
  EXPECT_THAT(queue.front(), Eq(sent.data));
}

TEST_F(Gossip, ShouldReturnErrorGivenUnresolvedHost) {
  const Payload sent("hello world!");
  Address address{"unresolved_host", 10086};

  auto result = peers_[0]->Gossip({address}, sent);

  EXPECT_THAT(result, Eq(ErrorCode::kHostNotFound));
}

TEST_F(Gossip, ShouldCallbackWithErrorGivenUnresolvedHostAsynchronously) {
  const Payload sent("hello world!");
  Address address{"unresolved_host", 10086};

  std::promise<ErrorCode> promise;
  auto result = peers_[0]->Gossip({address}, sent,
                                  [&](auto code) { promise.set_value(code); });

  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  auto future = promise.get_future();
  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
  EXPECT_THAT(future.get(), Eq(ErrorCode::kHostNotFound));
}

class Push : public Gossip {
 public:
  void SetUp() {
    local_ = CreateTransport(local_address_, local_address_);
    remote_ = CreateTransport(remote_address_, remote_address_);
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
  Address local_address_{"127.0.0.1", 5000};
  Address remote_address_{"127.0.0.1", 5001};
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

  auto result = local_->Push(remote_address_, sent.data(), sent.size());

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
  local_->Push(remote_address_, sent.data(), sent.size());

  auto result = local_->Push(remote_address_, sent.data(), sent.size());

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

  std::promise<void> promise;
  auto result = local_->Push(remote_address_, sent.data(), sent.size(),
                             [&](auto) { promise.set_value(); });

  {
    std::unique_lock<std::mutex> lock(mutex_);
    ASSERT_TRUE(
        cv_.wait_for(lock, kTimeout, [&]() { return !queue_.empty(); }));
  }
  EXPECT_THAT(queue_.front(), Eq(sent));
  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  auto future = promise.get_future();
  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
}

TEST_F(Push, ShouldReturnErrorGivenUnresolvedHost) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};
  Address address{"unresolved_host", 10086};

  auto result = local_->Push(address, sent.data(), sent.size());

  ASSERT_THAT(result, Eq(ErrorCode::kHostNotFound));
}

TEST_F(Push, ShouldReturnErrorGivenUnresolvedHostAsynchronously) {
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5};
  Address address{"unresolved_host", 10086};

  std::promise<ErrorCode> promise;
  auto result = local_->Push(address, sent.data(), sent.size(),
                             [&](auto code) { promise.set_value(code); });

  ASSERT_THAT(result, Eq(ErrorCode::kOK));
  auto future = promise.get_future();
  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
  EXPECT_THAT(future.get(), Eq(ErrorCode::kHostNotFound));
}

class Pull : public Gossip {
 public:
  void SetUp() {
    local_ = CreateTransport(local_address_, local_address_);
    remote_ = CreateTransport(remote_address_, remote_address_);
    remote_->RegisterPullHandler(
        [](const Address &, const void *, size_t) { return Pull::kResponse; });
  }

 protected:
  Address local_address_{"127.0.0.1", 5000};
  Address remote_address_{"127.0.0.1", 5001};
  std::unique_ptr<Transportable> local_;
  std::unique_ptr<Transportable> remote_;
  static constexpr const std::chrono::milliseconds kTimeout{
      std::chrono::milliseconds(1000)};
  static const std::vector<uint8_t> kRequest;
  static const std::vector<uint8_t> kResponse;
};

const std::vector<uint8_t> Pull::kRequest{1, 2, 3, 4, 5};
const std::vector<uint8_t> Pull::kResponse{5, 4, 3, 2, 1};

TEST_F(Pull, ShouldPullDataFromRemotePeer) {
  auto result = local_->Pull(remote_address_, kRequest.data(), kRequest.size());

  ASSERT_THAT(result.first, Eq(ErrorCode::kOK));
  EXPECT_THAT(result.second, Eq(kResponse));
}

TEST_F(Pull, ShouldPullDataFromRemotePeerAsynchronously) {
  using PullResult = gossip::Pullable::PullResult;
  std::promise<PullResult> promise;
  std::future<PullResult> future = promise.get_future();
  auto result = local_->Pull(remote_address_, kRequest.data(), kRequest.size(),
                             [&](auto &result) { promise.set_value(result); });

  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
  auto response = future.get();
  ASSERT_THAT(response.first, Eq(ErrorCode::kOK));
  EXPECT_THAT(response.second, Eq(kResponse));
}

TEST_F(Pull, ShouldReturnErrorGivenUnresolvedHost) {
  Address address{"unresolved_host", 10086};

  auto result = local_->Pull(address, kRequest.data(), kRequest.size());

  ASSERT_THAT(result.first, Eq(ErrorCode::kHostNotFound));
}

TEST_F(Pull, ShouldPullErrorGivenUnresolvedHostAsynchronously) {
  Address address{"unresolved_host", 10086};

  std::promise<gossip::Pullable::PullResult> promise;
  auto did_pull = [&](auto &result) { promise.set_value(result); };
  auto result =
      local_->Pull(address, kRequest.data(), kRequest.size(), did_pull);

  auto future = promise.get_future();
  ASSERT_THAT(future.wait_for(kTimeout), Eq(std::future_status::ready));
  ASSERT_THAT(future.get().first, Eq(ErrorCode::kHostNotFound));
}
