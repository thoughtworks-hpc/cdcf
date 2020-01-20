//
// Created by Zhao Wenbo on 2020/1/19.
//

#include "include/membership_message.h"

void membership::Message::InitAsUpMessage(const Member& member) {
  msg_.set_name(member.GetNodeName());
  msg_.set_ip(member.GetIpAddress());
  msg_.set_port(member.GetPort());
  msg_.set_status(MemberUpdate::UP);
  msg_.set_incarnation(1);
}

std::string membership::Message::SerializeToString() {
  return msg_.SerializeAsString();
}

void membership::Message::DeserializeFromString(const std::string& data) {
  msg_.ParseFromString(data);
}
membership::Member membership::Message::GetMember() {
  return Member{msg_.name(), msg_.ip(), static_cast<short>(msg_.port())};
}
bool membership::Message::IsUpMessage() {
  return !(!msg_.IsInitialized() || msg_.status() != MemberUpdate::UP);
}
