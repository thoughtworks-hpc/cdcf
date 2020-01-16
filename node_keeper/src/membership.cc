//
// Created by Zhao Wenbo on 2020/1/14.
//

#include <include/membership.h>

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

  return 0;
}
int membership::Membership::AddMember(const membership::Member& member) {
  // TODO considering nessicity of mutex here
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
  if (node_name_ == "" && ip_address_ == "" && port_ == 0) {
    return true;
  }

  return false;
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
