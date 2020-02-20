/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_INCLUDE_MEMBERSHIP_H_
#define NODE_KEEPER_INCLUDE_MEMBERSHIP_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "include/gossip.h"

namespace membership {

enum ErrorCode {
  MEMBERSHIP_SUCCESS,
  MEMBERSHIP_FAILURE,
  MEMBERSHIP_INIT_HOSTMEMBER_EMPTY,
  MEMBERSHIP_INIT_TRANSPORT_EMPTY
};

class Member {
 public:
  Member() : node_name_(""), ip_address_(""), port_(0) {}
  Member(std::string node_name, std::string ip_address, uint16_t port)
      : node_name_(std::move(node_name)),
        ip_address_(std::move(ip_address)),
        port_(port) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);

  std::string GetNodeName() const { return node_name_; }
  std::string GetIpAddress() const { return ip_address_; }
  uint16_t GetPort() const { return port_; }

  bool IsEmptyMember();

 private:
  std::string node_name_;
  std::string ip_address_;
  uint16_t port_;
};

struct MemberCompare {
  bool operator()(const Member& lhs, const Member& rhs) const {
    if (lhs.GetIpAddress() < rhs.GetIpAddress()) {
      return true;
    } else if (lhs.GetPort() < rhs.GetPort()) {
      return true;
    }
    return false;
  }
};

class Config {
 public:
  Config() : retransmit_multiplier_(1) {}

  int AddHostMember(const std::string& node_name, const std::string& ip_address,
                    uint16_t port);
  Member GetHostMember() const { return host_; }

  int AddOneSeedMember(const std::string& node_name,
                       const std::string& ip_address, uint16_t port);
  std::vector<Member> GetSeedMembers() const { return seed_members_; }

  void AddRetransmitMultiplier(int multiplier);
  int GetRetransmitMultiplier() const { return retransmit_multiplier_; }

 private:
  Member host_;
  std::vector<Member> seed_members_;
  int retransmit_multiplier_;
};

class Subscriber {
 public:
  void virtual Update() = 0;
};

class Membership {
 public:
  Membership() : retransmit_multiplier_(1), incarnation_(0) {}
  ~Membership();
  int Init(std::shared_ptr<gossip::Transportable> transport,
           const Config& config);
  std::vector<Member> GetMembers();
  void Subscribe(std::shared_ptr<Subscriber> subscriber);

 private:
  int AddMember(const Member& member);
  void MergeUpUpdate(const Member& member, unsigned int incarnation);
  void MergeDownUpdate(const Member& member, unsigned int incarnation);
  void Notify();
  void HandleGossip(const struct gossip::Address& node,
                    const gossip::Payload& payload);
  void HandlePush(const gossip::Address&, const void* data, size_t size);
  void HandlePull(const gossip::Address&, const void* data, size_t size);
  int GetRetransmitLimit();

  std::map<Member, int, MemberCompare> members_;
  std::mutex mutex_members_;
  Member self_;
  std::shared_ptr<gossip::Transportable> transport_;
  std::vector<std::shared_ptr<Subscriber>> subscribers_;
  unsigned int incarnation_;
  int retransmit_multiplier_;
};

};  // namespace membership

#endif  // NODE_KEEPER_INCLUDE_MEMBERSHIP_H_
