/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_GOSSIP_H_
#define NODE_KEEPER_SRC_GOSSIP_H_

#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace gossip {

struct Address {
  std::string host;
  unsigned short port;

  bool operator==(const Address &rhs) const {
    return host == rhs.host && port == rhs.port;
  }
};

class Payload {
 public:
  static constexpr int kMaxPayloadSize = 65527;

  explicit Payload(std::string data);
  Payload(char *data, int size);

 private:
  char data[kMaxPayloadSize];
};

class Gossipable {
 public:
  // payload dissemination via gossip protocol
  //  virtual int Gossip(const std::vector<Address> &nodes,
  //                     const Payload &data) = 0;

  virtual int Gossip(const std::vector<Address> &nodes, const void *data,
                     size_t size) = 0;

  typedef std::function<void(const struct Address &node, const Payload &data)>
      GossipHandler;
  virtual void RegisterGossipHandler(GossipHandler handler) = 0;
};

class Pushable {
  // public:
  //  virtual int Push(const Address &node, const void *data, size_t size) = 0;
  //
  //  typedef std::function<void(const Address &, const void *, size_t)>
  //      PushHandler;
  //  virtual void RegisterPushHandler(PushHandler handler) = 0;
};

class Pullable {
  // public:
  //  virtual int Pull(const Address &node, const void *data, size_t size) = 0;
  //
  //  typedef std::function<void(const Address &, const void *, size_t)>
  //      PullHandler;
  //  virtual void RegisterPullHandler(PullHandler handler) = 0;
};

class Transportable : public Gossipable, public Pushable, public Pullable {};

};      // namespace gossip
#endif  // NODE_KEEPER_SRC_GOSSIP_H_
