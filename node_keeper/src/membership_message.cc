/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "include/membership_message.h"

std::string membership::Message::SerializeToString() {
  return BaseMessage().SerializeAsString();
}

void membership::Message::DeserializeFromString(const std::string& data) {
  BaseMessage().ParseFromString(data);
}

void membership::Message::DeserializeFromArray(const void* data, int size) {
  BaseMessage().ParseFromArray(data, size);
}

void membership::UpdateMessage::InitAsUpMessage(const Member& member,
                                                unsigned int incarnation) {
  update_.set_name(member.GetNodeName());
  update_.set_ip(member.GetIpAddress());
  update_.set_port(member.GetPort());
  update_.set_status(MemberUpdate::UP);
  update_.set_incarnation(incarnation);
}

void membership::UpdateMessage::InitAsDownMessage(const Member& member,
                                                  unsigned int incarnation) {
  update_.set_name(member.GetNodeName());
  update_.set_ip(member.GetIpAddress());
  update_.set_port(member.GetPort());
  update_.set_status(MemberUpdate::DOWN);
  update_.set_incarnation(incarnation);
}

membership::Member membership::UpdateMessage::GetMember() const {
  return Member{update_.name(), update_.ip(),
                static_cast<__uint16_t>(update_.port())};
}

bool membership::UpdateMessage::IsUpMessage() const {
  return !(!update_.IsInitialized() || update_.status() != MemberUpdate::UP);
}

bool membership::UpdateMessage::IsDownMessage() const {
  return !(!update_.IsInitialized() || update_.status() != MemberUpdate::DOWN);
}

void membership::FullStateMessage::InitAsFullStateMessage(
    const std::vector<Member>& members) {
  for (auto member : members) {
    auto new_state = state_.add_states();
    new_state->set_name(member.GetNodeName());
    new_state->set_ip(member.GetIpAddress());
    new_state->set_port(member.GetPort());
    new_state->set_status(MemberUpdate::UP);
    new_state->set_incarnation(1);
  }
}

std::vector<membership::Member> membership::FullStateMessage::GetMembers() {
  std::vector<Member> members;
  for (auto state : state_.states()) {
    members.emplace_back(state.name(), state.ip(), state.port());
  }
  return members;
}
