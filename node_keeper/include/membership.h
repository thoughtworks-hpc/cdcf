//
// Created by Zhao Wenbo on 2020/1/14.
//

#ifndef CDCF_MEMBERSHIP_H
#define CDCF_MEMBERSHIP_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "include/gossip.h"
//#include "include/membership_message.h"

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
  Config() : transport_(nullptr), retransmit_multiplier_(1) {}

  int AddHostMember(const std::string& node_name, const std::string& ip_address,
                    short port);
  Member GetHostMember() const { return host_; }

  int AddOneSeedMember(const std::string& node_name,
                       const std::string& ip_address, short port);
  std::vector<Member> GetSeedMembers() const { return seed_members_; }

  int AddTransport(gossip::Transportable* transport);
  gossip::Transportable* GetTransport() const { return transport_; }

  void AddRetransmitMultiplier(int multiplier);
  int GetRetransmitMultiplier() const { return retransmit_multiplier_; }

 private:
  Member host_;
  std::vector<Member> seed_members_;
  gossip::Transportable* transport_;
  int retransmit_multiplier_;
};

class Membership {
 public:
  Membership()
      : transport_(nullptr), retransmit_multiplier_(1), incarnation_(0) {}
  int Init(const Config& config);
  std::vector<Member> GetMembers();

 private:
  // std::vector<Member> members_;
  std::map<Member, int, MemberCompare> members_;
  gossip::Transportable* transport_;
  unsigned int incarnation_;
  int retransmit_multiplier_;

  int AddMember(const Member& member);
  void IncrementIncarnation();
  void HandleGossip(const struct gossip::Address& node,
                    const gossip::Payload& payload);
};

};  // namespace membership

#endif  // CDCF_MEMBERSHIP_H
