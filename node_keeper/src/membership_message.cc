//
// Created by Zhao Wenbo on 2020/1/19.
//

#include "include/membership_message.h"

void membership::Message::InitAsUpMessage(const Member& member,
                                          unsigned int incarnation) {
  msg_.set_name(member.GetNodeName());
  msg_.set_ip(member.GetIpAddress());
  msg_.set_port(member.GetPort());
  msg_.set_status(MemberUpdate::UP);
  msg_.set_incarnation(incarnation);
}

void membership::Message::InitAsDownMessage(const Member& member,
                                            unsigned int incarnation) {
  msg_.set_name(member.GetNodeName());
  msg_.set_ip(member.GetIpAddress());
  msg_.set_port(member.GetPort());
  msg_.set_status(MemberUpdate::DOWN);
  msg_.set_incarnation(incarnation);
}

std::string membership::Message::SerializeToString() {
  return msg_.SerializeAsString();
}

void membership::Message::DeserializeFromString(const std::string& data) {
  msg_.ParseFromString(data);
}

void membership::Message::DeserializeFromArray(const void* data, int size) {
  msg_.ParseFromArray(data, size);
}

membership::Member membership::Message::GetMember() {
  return Member{msg_.name(), msg_.ip(), static_cast<short>(msg_.port())};
}

bool membership::Message::IsUpMessage() {
  return !(!msg_.IsInitialized() || msg_.status() != MemberUpdate::UP);
}

bool membership::Message::IsDownMessage() {
  return !(!msg_.IsInitialized() || msg_.status() != MemberUpdate::DOWN);
}