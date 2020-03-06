/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_INCLUDE_GOSSIP_H_
#define NODE_KEEPER_INCLUDE_GOSSIP_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gossip {

enum ErrorCode {
  kOK = 0,
  kUnknown,
};

struct Address {
  std::string host;
  uint16_t port;

  bool operator==(const Address &rhs) const {
    return host == rhs.host && port == rhs.port;
  }

  bool operator!=(const Address &rhs) const {
    return host != rhs.host || port != rhs.port;
  }
};

class Payload {
 public:
  class MaxPayloadExceeded : public std::runtime_error {
   public:
    explicit MaxPayloadExceeded(size_t actual)
        : std::runtime_error(
              "max payload length is: " + std::to_string(kMaxPayloadSize) +
              ", but " + std::to_string(actual) + " is provided.") {}
  };

  static constexpr int kMaxPayloadSize = 65527;

  std::vector<uint8_t> data;

  explicit Payload(const std::string &data)
      : Payload(data.c_str(), data.size()) {}

  explicit Payload(const std::vector<uint8_t> &data)
      : Payload(data.data(), data.size()) {}

  Payload(const void *data, size_t size) {
    if (size > kMaxPayloadSize) {
      throw MaxPayloadExceeded(size);
    }
    auto begin = reinterpret_cast<const uint8_t *>(data);
    this->data.assign(begin, begin + size);
  }
};

class Gossipable {
 public:
  // payload dissemination via gossip protocol
  typedef std::function<void(ErrorCode)> DidGossipHandler;
  virtual ErrorCode Gossip(const std::vector<Address> &nodes,
                           const Payload &data,
                           DidGossipHandler didGossip = nullptr) = 0;

  typedef std::function<void(const Address &node, const Payload &data)>
      GossipHandler;
  virtual void RegisterGossipHandler(GossipHandler handler) = 0;

 public:
  virtual ~Gossipable() = default;
};

class Pushable {
 public:
  typedef std::function<void(ErrorCode)> DidPushHandler;
  virtual ErrorCode Push(const Address &node, const void *data, size_t size,
                         DidPushHandler didPush = nullptr) = 0;

  typedef std::function<void(const Address &, const void *, size_t)>
      PushHandler;
  virtual void RegisterPushHandler(PushHandler handler) = 0;

 public:
  virtual ~Pushable() = default;
};

class Pullable {
 public:
  typedef std::pair<ErrorCode, std::vector<uint8_t>> PullResult;
  typedef std::function<void(const PullResult &)> DidPullHandler;
  virtual PullResult Pull(const Address &node, const void *data, size_t size,
                          DidPullHandler didPull = nullptr) = 0;

  typedef std::function<std::vector<uint8_t>(const Address &, const void *,
                                             size_t)>
      PullHandler;
  virtual void RegisterPullHandler(PullHandler handler) = 0;

 public:
  virtual ~Pullable() = default;
};

class Transportable : public Gossipable, public Pushable, public Pullable {};

class PortOccupied : public std::runtime_error {
 public:
  explicit PortOccupied(const std::exception &e)
      : std::runtime_error(e.what()) {}
};

std::unique_ptr<Transportable> CreateTransport(const Address &upd,
                                               const Address &tcp);
};      // namespace gossip
#endif  // NODE_KEEPER_INCLUDE_GOSSIP_H_
