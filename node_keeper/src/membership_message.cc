/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/membership_message.h"

std::string membership::Message::SerializeToString() {
  return BaseMessage().SerializeAsString();
}

void membership::Message::DeserializeFromString(const std::string& data) {
  BaseMessage().ParseFromString(data);
}

void membership::Message::DeserializeFromArray(const void* data, int size) {
  BaseMessage().ParseFromArray(data, size);
}

void membership::UpdateMessage::SetUpdate(const Member& member,
                                          unsigned int incarnation) {
  update_.set_name(member.GetNodeName());
  update_.set_hostname(member.GetHostName());
  update_.set_ip(member.GetIpAddress());
  update_.set_port(member.GetPort());
  update_.set_incarnation(incarnation);
  update_.set_member_id(member.GetUid());
  update_.set_role(member.GetRole());
}

void membership::UpdateMessage::InitAsUpMessage(const Member& member,
                                                unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::UP);
}

void membership::UpdateMessage::InitAsDownMessage(const Member& member,
                                                  unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::DOWN);
}

void membership::UpdateMessage::InitAsSuspectMessage(const Member& member,
                                                     unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::SUSPECT);
}

void membership::UpdateMessage::InitAsRecoveryMessage(
    const Member& member, unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::RECOVERY);
}

membership::Member membership::UpdateMessage::GetMember() const {
  return Member{update_.name(),
                update_.ip(),
                static_cast<uint16_t>(update_.port()),
                update_.hostname(),
                update_.member_id(),
                update_.role()};
}

bool membership::UpdateMessage::IsUpMessage() const {
  return !(!update_.IsInitialized() || update_.status() != MemberUpdate::UP);
}

bool membership::UpdateMessage::IsDownMessage() const {
  return !(!update_.IsInitialized() || update_.status() != MemberUpdate::DOWN);
}

bool membership::UpdateMessage::IsSuspectMessage() const {
  return !(!update_.IsInitialized() ||
           update_.status() != MemberUpdate::SUSPECT);
}

bool membership::UpdateMessage::IsRecoveryMessage() const {
  return !(!update_.IsInitialized() ||
           update_.status() != MemberUpdate::RECOVERY);
}

bool membership::UpdateMessage::IsActorSystemDownMessage() const {
  if (!update_.IsInitialized()) {
    return false;
  }
  return update_.status() == MemberUpdate::ACTOR_SYSTEM_DOWN;
}

bool membership::UpdateMessage::IsActorSystemUpMessage() const {
  if (!update_.IsInitialized()) {
    return false;
  }
  return update_.status() == MemberUpdate::ACTOR_SYSTEM_UP;
}

void membership::FullStateMessage::InitAsFullStateMessage(
    const std::vector<Member>& members) {
  for (const auto& member : members) {
  CDCF_LOGGER_DEBUG("---------------------------------");
  CDCF_LOGGER_DEBUG("InitAsFullStateMessage: ");
  for (const auto member: members) {
      CDCF_LOGGER_DEBUG("After members.emplace_back() ");
      CDCF_LOGGER_DEBUG("name: {}", member.GetNodeName());
      CDCF_LOGGER_DEBUG("hostname: {}", member.GetHostName());
      CDCF_LOGGER_DEBUG("ip: {}", member.GetIpAddress());
      CDCF_LOGGER_DEBUG("role: {}", member.GetRole());
      CDCF_LOGGER_DEBUG("uid: {}", member.GetUid());
  }
    state_.set_error(MemberFullState::SUCCESS);
    auto new_state = state_.add_states();
    new_state->set_name(member.GetNodeName());
    new_state->set_hostname(member.GetHostName());
    new_state->set_ip(member.GetIpAddress());
    new_state->set_role(member.GetRole());
    new_state->set_port(member.GetPort());
    new_state->set_status(MemberUpdate::UP);
    new_state->set_incarnation(1);
  }
}

void membership::UpdateMessage::InitAsActorSystemDownMessage(
    const membership::Member& member, unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::ACTOR_SYSTEM_DOWN);
}

void membership::UpdateMessage::InitAsActorSystemUpMessage(
    const Member& member, unsigned int incarnation) {
  SetUpdate(member, incarnation);
  update_.set_status(MemberUpdate::ACTOR_SYSTEM_UP);
}

void membership::FullStateMessage::InitAsReentryRejected() {
  state_.set_error(MemberFullState::REENTRY_REJECTED);
}

std::vector<membership::Member> membership::FullStateMessage::GetMembers() {
  std::vector<Member> members;
  for (const auto& state : state_.states()) {
    members.emplace_back(state.name(), state.ip(), state.port(),
                         state.hostname(), "", state.role());
  }
  return members;
}

bool membership::FullStateMessage::IsSuccess() {
  return state_.error() == MemberFullState::SUCCESS;
}

void membership::PullRequestMessage::InitAsFullStateType() {
  pull_request_.set_type(
      ::membership::PullRequest_Type::PullRequest_Type_FULL_STATE);
}

void membership::PullRequestMessage::InitAsPingType() {
  pull_request_.set_type(::membership::PullRequest_Type::PullRequest_Type_PING);
}

// Todo(davidzwb): consider using md5 to optimize message size
void membership::PullRequestMessage::InitAsPingType(
    const std::map<Member, int>& members,
    const std::map<Member, bool>& member_actor_system) {
  pull_request_.set_type(::membership::PullRequest_Type::PullRequest_Type_PING);

  for (const auto& member_pair : members) {
    auto member = member_pair.first;
    auto incarnation = member_pair.second;
    auto new_state = pull_request_.add_states();
    new_state->set_name(member.GetNodeName());
    new_state->set_hostname(member.GetHostName());
    new_state->set_ip(member.GetIpAddress());
    new_state->set_port(member.GetPort());
    new_state->set_role(member.GetRole());
    new_state->set_status(MemberUpdate::UP);
    new_state->set_incarnation(incarnation);
    if (auto it = member_actor_system.find(member);
        it != member_actor_system.end()) {
      new_state->set_actor_system_up(it->second);
    } else {
      new_state->set_actor_system_up(false);
    }
  }
}

void membership::PullRequestMessage::InitAsPingRelayType(const Member& self,
                                                         const Member& target) {
  pull_request_.set_type(
      ::membership::PullRequest_Type::PullRequest_Type_PING_RELAY);
  pull_request_.set_name(target.GetNodeName());
  pull_request_.set_ip(target.GetIpAddress());
  pull_request_.set_port(target.GetPort());
  pull_request_.set_self_name(self.GetNodeName());
  pull_request_.set_self_ip(self.GetIpAddress());
  pull_request_.set_self_port(self.GetPort());
}

bool membership::PullRequestMessage::IsFullStateType() {
  return pull_request_.type() == PullRequest_Type_FULL_STATE;
}

bool membership::PullRequestMessage::IsPingType() {
  return pull_request_.type() == PullRequest_Type_PING;
}

bool membership::PullRequestMessage::IsPingRelayType() {
  return pull_request_.type() == PullRequest_Type_PING_RELAY;
}

std::string membership::PullRequestMessage::GetName() {
  std::string name;
  if (IsPingRelayType()) {
    name = pull_request_.name();
  }
  return name;
}

std::string membership::PullRequestMessage::GetIpAddress() {
  std::string ip;
  if (IsPingRelayType()) {
    ip = pull_request_.ip();
  }
  return ip;
}

unsigned int membership::PullRequestMessage::GetPort() {
  int port = 0;
  if (IsPingRelayType()) {
    port = pull_request_.port();
  }
  return port;
}

std::string membership::PullRequestMessage::GetSelfIpAddress() {
  std::string ip;
  if (IsPingRelayType()) {
    ip = pull_request_.self_ip();
  }
  return ip;
}

unsigned int membership::PullRequestMessage::GetSelfPort() {
  int port = 0;
  if (IsPingRelayType()) {
    port = pull_request_.self_port();
  }
  return port;
}

std::map<membership::Member, int>
membership::PullRequestMessage::GetMembersWithIncarnation() {
  std::map<membership::Member, int> members;
  for (int i = 0; i < pull_request_.states_size(); i++) {
    auto update = pull_request_.states(i);
    if (update.status() == MemberUpdate::UP) {
      membership::Member member{update.name(),
                                update.ip(),
                                static_cast<uint16_t>(update.port()),
                                update.hostname(),
                                update.member_id(),
                                update.role()};
      members[member] = update.incarnation();
    }
  }
  return members;
}

std::map<membership::Member, bool>
membership::PullRequestMessage::GetMembersWithActorSystem() {
  std::map<membership::Member, bool> member_actor_system;
  for (int i = 0; i < pull_request_.states_size(); i++) {
    auto update = pull_request_.states(i);
    if (update.status() == MemberUpdate::UP) {
      membership::Member member{update.name(),
                                update.ip(),
                                static_cast<uint16_t>(update.port()),
                                update.hostname(),
                                update.member_id(),
                                update.role()};
      member_actor_system[member] = update.actor_system_up();
    }
  }
  return member_actor_system;
}

void membership::PullResponseMessage::InitAsPingSuccess(const Member& member) {
  pull_response_.set_type(PullResponse_Type_PING_SUSCESS);
  pull_response_.set_name(member.GetNodeName());
  pull_response_.set_ip(member.GetIpAddress());
  pull_response_.set_port(member.GetPort());
}

void membership::PullResponseMessage::InitAsPingFailure(const Member& member) {
  pull_response_.set_type(PullResponse_Type_PING_FAILURE);
  pull_response_.set_name(member.GetNodeName());
  pull_response_.set_ip(member.GetIpAddress());
  pull_response_.set_port(member.GetPort());
}

void membership::PullResponseMessage::InitAsPingReceived() {
  pull_response_.set_type(PullResponse_Type_PING_REQUEST_RECEIVED);
}

membership::Member membership::PullResponseMessage::GetMember() {
  if (pull_response_.type() == PullResponse_Type_PING_SUSCESS ||
      pull_response_.type() == PullResponse_Type_PING_FAILURE) {
    Member member{pull_response_.name(), pull_response_.ip(),
                  static_cast<uint16_t>(pull_response_.port())};
    return member;
  } else {
    return Member();
  }
}

bool membership::PullResponseMessage::IsPingSuccess() {
  return pull_response_.type() == PullResponse_Type_PING_SUSCESS;
}

bool membership::PullResponseMessage::IsPingFailure() {
  return pull_response_.type() == PullResponse_Type_PING_FAILURE;
}
