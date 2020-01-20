//
// Created by Zhao Wenbo on 2020/1/14.
//

#include <include/membership.h>

#include <iostream>
#include <string>
#include <vector>

std::vector<membership::Member> membership::Membership::GetMembers() {
  Member member("node1", "127.0.0.1", 27777);
  return members_;
}
int membership::Membership::Init(Config config) {
  // TODO existence checking
  // TODO host member further checking

  Member member = config.GetHostMember();
  if (member.IsEmptyMember()) {
    return MEMBERSHIP_INIT_HOSTMEMBER_EMPTY;
  }
  AddMember(member);

  if (config.GetTransport() == nullptr) {
    return MEMBERSHIP_INIT_TRANSPORT_EMPTY;
  }

  transport_ = config.GetTransport();

  transport_->RegisterGossipHandler(
      [&](const struct gossip::Address& node, const gossip::Payload& payload) {
        std::cout << "hello" << std::endl;
      });
  return 0;
}
int membership::Membership::AddMember(const membership::Member& member) {
  // TODO considering necessity of mutex here
  members_.push_back(member);

  return 0;
}
bool membership::operator==(const membership::Member& lhs,
                            const membership::Member& rhs) {
  return !((lhs.GetIpAddress() != rhs.GetIpAddress()) ||
           (lhs.GetPort() != rhs.GetPort()));
}
bool membership::operator!=(const membership::Member& lhs,
                            const membership::Member& rhs) {
  return !operator==(lhs, rhs);
}
bool membership::Member::IsEmptyMember() {
  return node_name_.empty() && ip_address_.empty() && port_ == 0;
}
int membership::Config::AddHostMember(const std::string& node_name,
                                      const std::string& ip_address,
                                      short port) {
  Member host(node_name, ip_address, port);
  host_ = host;

  return 0;
}
int membership::Config::AddOneSeedMember(const std::string& node_name,
                                         const std::string& ip_address,
                                         short port) {
  Member seed(node_name, ip_address, port);

  // TODO existing member check

  seed_members_.push_back(seed);

  return 0;
}
int membership::Config::AddTransport(gossip::Transportable* transport) {
  assert(transport != nullptr);

  transport_ = transport;
  return 0;
}
