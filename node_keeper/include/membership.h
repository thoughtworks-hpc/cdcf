//
// Created by Zhao Wenbo on 2020/1/14.
//

#ifndef CDCF_MEMBERSHIP_H
#define CDCF_MEMBERSHIP_H

#include <string>
#include <unordered_map>
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
  Member(std::string node_name, std::string ip_address, short port)
      : node_name_(std::move(node_name)),
        ip_address_(std::move(ip_address)),
        port_(port) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);

  std::string GetNodeName() const { return node_name_; }
  std::string GetIpAddress() const { return ip_address_; }
  short GetPort() const { return port_; }

  bool IsEmptyMember();

 private:
  std::string node_name_;
  std::string ip_address_;
  short port_;
};

class Config {
 public:
  Config() : transport_(nullptr) {}

  int AddHostMember(const std::string& node_name, const std::string& ip_address,
                    short port);
  Member GetHostMember() { return host_; }

  int AddOneSeedMember(const std::string& node_name,
                       const std::string& ip_address, short port);
  std::vector<Member> GetSeedMembers() { return seed_members_; }

  int AddTransport(gossip::Transportable* transport);
  gossip::Transportable* GetTransport() { return transport_; }

 private:
  Member host_;
  std::vector<Member> seed_members_;
  gossip::Transportable* transport_;
};

class Membership {
 public:
  Membership() : transport_(nullptr) {}
  int Init(Config config);
  std::vector<Member> GetMembers();

 private:
  int AddMember(const Member& member);
  // int JoinCluster(const std::vector<Member>& seed_nodes);
  std::vector<Member> members_;
  gossip::Transportable* transport_;
};

};  // namespace membership

#endif  // CDCF_MEMBERSHIP_H
