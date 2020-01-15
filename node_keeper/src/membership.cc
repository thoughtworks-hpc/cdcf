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
int membership::Membership::Init(const std::string& node_name,
                                 const std::string& ip_address, short port) {
  Member self_member(node_name, ip_address, port);

  // TODO existence checking

  addMember(self_member);

  return 0;
}
int membership::Membership::addMember(const membership::Member& member) {
  members_.push_back(member);

  return 0;
}
bool membership::operator==(const membership::Member& lhs,
                            const membership::Member& rhs) {
  if (lhs.GetIpAddress() != rhs.GetIpAddress()) {
    return false;
  }
  if (lhs.GetPort() != rhs.GetPort()) {
    return false;
  }

  return true;
}
bool membership::operator!=(const membership::Member& lhs,
                            const membership::Member& rhs) {
  return !operator==(lhs, rhs);
}
