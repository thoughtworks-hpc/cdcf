//
// Created by Zhao Wenbo on 2020/1/14.
//

#ifndef CDCF_MEMBERSHIP_H
#define CDCF_MEMBERSHIP_H

#include <string>
#include <unordered_map>
#include <vector>

#include "include/gossip.h"

namespace membership {

// TODO Error Code

class Member {
 private:
  std::string node_name_;
  std::string ip_address_;
  short port_;

 public:
  Member() : node_name_(""), ip_address_(""), port_(0) {}
  Member(const std::string& node_name, const std::string& ip_address,
         short port)
      : node_name_(node_name), ip_address_(ip_address), port_(port) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);

  const std::string& GetNodeName() const { return node_name_; }
  const std::string& GetIpAddress() const { return ip_address_; }
  short GetPort() const { return port_; }
};

class Config {
 public:
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
  int Init(Config config);
  std::vector<Member> GetMembers();

 private:
  int AddMember(const Member& member);
  int JoinCluster(const std::vector<Member>& seed_nodes);
  std::vector<Member> members_;
};

};  // namespace membership

#endif  // CDCF_MEMBERSHIP_H
