/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_H_
#define NODE_KEEPER_SRC_GOSSIP_H_

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gossip {

enum ErrorCode {
  kOK = 0,
  kTimeout,
};

struct Address {
  std::string host;
  unsigned short port;
};

class Payload {
 public:
  static constexpr int kMaxPayloadSize = 65527;

  explicit Payload(std::string data);
  Payload(char *data, int size);

 private:
  std::vector<unsigned char> data_;
};

class Gossipable {
 public:
  // payload dissemination via gossip protocol
  virtual int Gossip(const std::vector<Address> &nodes,
                     const Payload &data) = 0;

  typedef std::function<void(const Address &node, const Payload &data)>
      GossipHandler;
  virtual void RegisterGossipHandler(GossipHandler handler) = 0;

 public:
  virtual ~Gossipable() = default;
};

class Pushable {
 public:
  typedef std::function<void(ErrorCode)> DidPushHandler;
  virtual ErrorCode Push(
      const Address &node, const void *data, size_t size,
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0},
      DidPushHandler didPush = nullptr) = 0;

  typedef std::function<void(const Address &, const void *, size_t)>
      PushHandler;
  virtual void RegisterPushHandler(PushHandler handler) = 0;

 public:
  virtual ~Pushable() = default;
};

class Pullable {
 public:
  typedef std::pair<ErrorCode, std::vector<unsigned char>> PullResult;
  typedef std::function<void(PullResult)> DidPullHandler;
  virtual PullResult Pull(
      const Address &node, const void *data, size_t size,
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0},
      DidPullHandler didPull = nullptr) = 0;

  typedef std::function<void(const Address &, const void *, size_t)>
      PullHandler;
  virtual void RegisterPullHandler(PullHandler handler) = 0;

 public:
  virtual ~Pullable() = default;
};

class Transportable : public Gossipable, public Pushable, public Pullable {};

};      // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_H_
