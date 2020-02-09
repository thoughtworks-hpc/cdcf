//
// Created by Zhao Wenbo on 2020/1/14.
//

#include "include/membership.h"

#include <string>
#include <vector>

#include "include/membership_message.h"

std::vector<membership::Member> membership::Membership::GetMembers() {
  std::vector<Member> return_members;
  for (auto& member : members_) {
    return_members.push_back(member.first);
  }
  return return_members;
}

int membership::Membership::Init(const Config& config) {
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
        HandleGossip(node, payload);
      });

  retransmit_multiplier_ = config.GetRetransmitMultiplier();

  if (!config.GetSeedMembers().empty()) {
    std::vector<gossip::Address> addresses;
    for (const auto& member : config.GetSeedMembers()) {
      addresses.emplace_back(
          gossip::Address{member.GetIpAddress(), member.GetPort()});
    }

    membership::Message message;
    message.InitAsUpMessage(member, 1);
    std::string serialized_msg = message.SerializeToString();
    gossip::Payload payload(serialized_msg);

    transport_->Gossip(addresses, payload);
  }

  return MEMBERSHIP_SUCCESS;
}

int membership::Membership::AddMember(const membership::Member& member) {
  // TODO considering necessity of mutex here
  members_[member] = incarnation_;
  IncrementIncarnation();

  return 0;
}

void membership::Membership::HandleGossip(const struct gossip::Address& node,
                                          const gossip::Payload& payload) {
  membership::Message message;
  message.DeserializeFromArray(payload.data.data(), payload.data.size());

  int retrans_limit = retransmit_multiplier_ * ceil(log(members_.size()));

  if (retrans_limit > 0) {
    std::vector<gossip::Address> addresses;
    for (const auto& member : members_) {
      addresses.emplace_back(
          gossip::Address{member.first.GetIpAddress(), member.first.GetPort()});
    }
    while (retrans_limit > 0) {
      transport_->Gossip(addresses, payload);
      retrans_limit--;
    }
  }

  // message validity check

  if (message.IsUpMessage()) {
    if (members_.find(message.GetMember()) != members_.end()) {
      if (members_[message.GetMember()] < message.GetIncarnation()) {
        members_[message.GetMember()] = message.GetIncarnation();
      }
    } else {
      members_[message.GetMember()] = message.GetIncarnation();
    }
  } else if (message.IsDownMessage()) {
    members_.erase(message.GetMember());
  } else {
    return;
  }
}

void membership::Membership::IncrementIncarnation() {
  // TODO refactor to use atomic increment
  incarnation_++;
}

bool membership::operator==(const membership::Member& lhs,
                            const membership::Member& rhs) {
  if (lhs.GetIpAddress() == rhs.GetIpAddress() &&
      lhs.GetPort() == rhs.GetPort()) {
    return true;
  } else {
    return false;
  }
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

void membership::Config::AddRetransmitMultiplier(int multiplier) {
  retransmit_multiplier_ = multiplier;
}