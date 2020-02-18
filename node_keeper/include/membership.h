//
// Created by Zhao Wenbo on 2020/1/14.
//

#ifndef CDCF_MEMBERSHIP_H
#define CDCF_MEMBERSHIP_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "include/gossip.h"

namespace membership {

// TODO usage of MEMBERSHIP_INIT_TRANSPORT_EMPTY
enum ErrorCode {
  MEMBERSHIP_SUCCESS,
  MEMBERSHIP_FAILURE,
  MEMBERSHIP_INIT_HOSTMEMBER_EMPTY,
  MEMBERSHIP_INIT_TRANSPORT_EMPTY
};

class Member {
 public:
  Member() : node_name_(""), ip_address_(""), port_(0) {}
  Member(std::string node_name, std::string ip_address, short port)
      : node_name_(std::move(node_name)),
        ip_address_(std::move(ip_address)),
        port_(port) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);

  std::string GetNodeName() const { return node_name_; }
  std::string GetIpAddress() const { return ip_address_; }
  unsigned short GetPort() const { return port_; }

  bool IsEmptyMember();

 private:
  std::string node_name_;
  std::string ip_address_;
  unsigned short port_;
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
                    short port);
  Member GetHostMember() const { return host_; }

  int AddOneSeedMember(const std::string& node_name,
                       const std::string& ip_address, short port);
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

// TODO member leave scenario
class Membership {
 public:
  Membership() : retransmit_multiplier_(1), incarnation_(0) {}
  int Init(std::shared_ptr<gossip::Transportable> transport,
           const Config& config);
  std::vector<Member> GetMembers();
  void Subscribe(std::shared_ptr<Subscriber> subscriber);

 private:
  int AddMember(const Member& member);
  void Notify();
  void IncrementIncarnation();
  void HandleGossip(const struct gossip::Address& node,
                    const gossip::Payload& payload);
  void HandlePush(const gossip::Address&, const void* data, size_t size);
  void HandlePull(const gossip::Address&, const void* data, size_t size);

  // std::vector<Member> members_;
  // TODO need to use a mutex to protect members_
  std::map<Member, int, MemberCompare> members_;
  std::shared_ptr<gossip::Transportable> transport_;
  std::vector<std::shared_ptr<Subscriber>> subscribers_;
  unsigned int incarnation_;
  int retransmit_multiplier_;
};

};  // namespace membership

#endif  // CDCF_MEMBERSHIP_H
