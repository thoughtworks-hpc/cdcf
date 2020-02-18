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

int membership::Membership::Init(
    std::shared_ptr<gossip::Transportable> transport, const Config& config) {
  // TODO existence checking
  // TODO host member further checking

  Member member = config.GetHostMember();
  if (member.IsEmptyMember()) {
    return MEMBERSHIP_INIT_HOSTMEMBER_EMPTY;
  }
  AddMember(member);

  if (transport == nullptr) {
    return MEMBERSHIP_INIT_TRANSPORT_EMPTY;
  }

  transport_ = transport;

  auto gossip_handler = [this](const struct gossip::Address& node,
                               const gossip::Payload& payload) {
    HandleGossip(node, payload);
  };
  transport_->RegisterGossipHandler(gossip_handler);

  auto push_handler = [this](const gossip::Address& address, const void* data,
                             size_t size) { HandlePush(address, data, size); };
  transport_->RegisterPushHandler(push_handler);

  auto pull_handler = [this](const gossip::Address& address, const void* data,
                             size_t size) -> std::vector<uint8_t> {
    HandlePull(address, data, size);
    return std::vector<uint8_t>{0};
  };
  transport_->RegisterPullHandler(pull_handler);

  retransmit_multiplier_ = config.GetRetransmitMultiplier();

  if (!config.GetSeedMembers().empty()) {
    auto peer = config.GetSeedMembers()[0];
    gossip::Address address{peer.GetIpAddress(), peer.GetPort()};

    std::string pull_request_message = "pull";
    auto pull_request = [](const gossip::Pullable::PullResult&) {};

    transport_->Pull(address, pull_request_message.data(),
                     pull_request_message.size(), pull_request);
  }

  return MEMBERSHIP_SUCCESS;
}

int membership::Membership::AddMember(const membership::Member& member) {
  // TODO considering necessity of mutex here
  members_[member] = incarnation_;
  IncrementIncarnation();
  Notify();

  return 0;
}

void membership::Membership::HandleGossip(const struct gossip::Address& node,
                                          const gossip::Payload& payload) {
  membership::UpdateMessage message;
  message.DeserializeFromArray(payload.data.data(), payload.data.size());

  int retrans_limit = retransmit_multiplier_ * ceil(log(members_.size()));

  if (retrans_limit > 0) {
    std::vector<gossip::Address> addresses;
    for (const auto& member : members_) {
      addresses.emplace_back(
          gossip::Address{member.first.GetIpAddress(), member.first.GetPort()});
    }

    auto did_gossip = [](gossip::ErrorCode error) {};

    while (retrans_limit > 0) {
      transport_->Gossip(addresses, payload, did_gossip);
      retrans_limit--;
    }
  }

  if (message.IsUpMessage()) {
    if (members_.find(message.GetMember()) != members_.end()) {
      if (members_[message.GetMember()] < message.GetIncarnation()) {
        members_[message.GetMember()] = message.GetIncarnation();
      }
    } else {
      members_[message.GetMember()] = message.GetIncarnation();
    }
    Notify();
  } else if (message.IsDownMessage()) {
    members_.erase(message.GetMember());
    Notify();
  } else {
    return;
  }
}

void membership::Membership::HandlePush(const gossip::Address& address,
                                        const void* data, size_t size) {
  FullStateMessage message;
  message.DeserializeFromArray(data, size);

  for (const auto& member : message.GetMembers()) {
    AddMember(member);
  }
}

void membership::Membership::HandlePull(const gossip::Address& address,
                                        const void* data, size_t size) {
  FullStateMessage message;
  std::vector<Member> members;
  for (const auto& member : members_) {
    members.emplace_back(member.first.GetNodeName(),
                         member.first.GetIpAddress(), member.first.GetPort());
  }
  message.InitAsFullStateMessage(members);
  auto message_serialized = message.SerializeToString();

  auto did_push = [this, address, message_serialized](gossip::ErrorCode error) {
    if (error != gossip::ErrorCode::kOK) {
      transport_->Push(address, message_serialized.data(),
                       message_serialized.size());
    }
  };
  transport_->Push(address, message_serialized.data(),
                   message_serialized.size(), did_push);
}

void membership::Membership::IncrementIncarnation() {
  // TODO refactor to use atomic increment
  incarnation_++;
}
void membership::Membership::Subscribe(std::shared_ptr<Subscriber> subscriber) {
  subscribers_.push_back(subscriber);
}
void membership::Membership::Notify() {
  for (auto subscriber : subscribers_) {
    subscriber->Update();
  }
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

void membership::Config::AddRetransmitMultiplier(int multiplier) {
  retransmit_multiplier_ = multiplier;
}