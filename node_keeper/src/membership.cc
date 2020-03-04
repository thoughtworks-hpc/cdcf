/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "include/membership.h"

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "include/membership_message.h"


membership::Membership::~Membership() {
  if (members_.size() > 1) {
    UpdateMessage message;
    message.InitAsDownMessage(self_, incarnation_ + 1);
    auto message_serialized = message.SerializeToString();
    gossip::Payload payload{message_serialized};

    std::vector<gossip::Address> addresses;
    for (const auto& member : members_) {
      addresses.emplace_back(
          gossip::Address{member.first.GetIpAddress(), member.first.GetPort()});
    }

    int retransmit_limit = GetRetransmitLimit();
    while (retransmit_limit > 0) {
      transport_->Gossip(addresses, payload);
      retransmit_limit--;
    }
  }
}

std::vector<membership::Member> membership::Membership::GetMembers() {
  std::vector<Member> return_members;
  for (auto& member : members_) {
    return_members.push_back(member.first);
  }
  return return_members;
}

int membership::Membership::Init(
    std::shared_ptr<gossip::Transportable> transport, const Config& config) {
  // TODO(davidzwb): existence checking
  // TODO(davidzwb): host member further checking

  Member member = config.GetHostMember();
  if (member.IsEmptyMember()) {
    return MEMBERSHIP_INIT_HOSTMEMBER_EMPTY;
  }
  self_ = member;
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

  auto pull_handler = [this](const gossip::Address& address, const void* data,
                             size_t size) -> std::vector<uint8_t> {
    return HandlePull(address, data, size);
  };
  transport_->RegisterPullHandler(pull_handler);

  retransmit_multiplier_ = config.GetRetransmitMultiplier();

  if (!config.GetSeedMembers().empty()) {
    seed_members_ = config.GetSeedMembers();
    PullFromSeedMember();
  }

  return MEMBERSHIP_SUCCESS;
}

void membership::Membership::PullFromSeedMember() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, seed_members_.size() - 1);

  auto peer = seed_members_[dis(gen)];
  gossip::Address address{peer.GetIpAddress(), peer.GetPort()};

  std::string pull_request_message = "pull";

  transport_->Pull(address, pull_request_message.data(),
                   pull_request_message.size(),
                   [this](const gossip::Transportable::PullResult& result) {
                     if (result.first == gossip::ErrorCode::kOK) {
                       HandleDidPull(result);
                       return;
                     }
                     PullFromSeedMember();
                   });
}

int membership::Membership::AddMember(const membership::Member& member) {
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    members_[member] = incarnation_;
  }
  Notify();

  return 0;
}

void membership::Membership::HandleGossip(const struct gossip::Address& node,
                                          const gossip::Payload& payload) {
  membership::UpdateMessage message;
  message.DeserializeFromArray(payload.data.data(), payload.data.size());

  DisseminateGossip(payload);

  if (message.IsUpMessage()) {
    MergeUpUpdate(message.GetMember(), message.GetIncarnation());
  } else if (message.IsDownMessage()) {
    MergeDownUpdate(message.GetMember(), message.GetIncarnation());
  }
}

void membership::Membership::HandleDidPull(
    const gossip::Transportable::PullResult& result) {
  if (result.first == gossip::ErrorCode::kOK) {
    FullStateMessage message;
    message.DeserializeFromArray(result.second.data(), result.second.size());

    for (const auto& member : message.GetMembers()) {
      MergeUpUpdate(member, 0);
    }

    UpdateMessage update;
    update.InitAsUpMessage(self_, incarnation_);
    auto update_serialized = update.SerializeToString();
    gossip::Payload payload(update_serialized.data(), update_serialized.size());

    DisseminateGossip(payload);
  }
}

void membership::Membership::DisseminateGossip(const gossip::Payload& payload) {
  int retransmit_limit = GetRetransmitLimit();
  if (retransmit_limit <= 0) {
    return;
  }

  std::vector<gossip::Address> addresses;
  for (const auto& member : this->members_) {
    addresses.emplace_back(
        gossip::Address{member.first.GetIpAddress(), member.first.GetPort()});
  }

  auto did_gossip = [](gossip::ErrorCode error) {};

  while (retransmit_limit > 0) {
    this->transport_->Gossip(addresses, payload, did_gossip);
    retransmit_limit--;
  }
}

std::vector<uint8_t> membership::Membership::HandlePull(
    const gossip::Address& address, const void* data, size_t size) {
  FullStateMessage message;
  std::vector<Member> members;
  for (const auto& member : members_) {
    members.emplace_back(member.first.GetNodeName(),
                         member.first.GetIpAddress(), member.first.GetPort());
  }
  message.InitAsFullStateMessage(members);
  auto message_serialized = message.SerializeToString();

  return std::vector<uint8_t>(message_serialized.begin(),
                              message_serialized.end());
}

void membership::Membership::Subscribe(std::shared_ptr<Subscriber> subscriber) {
  subscribers_.push_back(subscriber);
}
void membership::Membership::Notify() {
  for (auto subscriber : subscribers_) {
    subscriber->Update();
  }
}

void membership::Membership::MergeUpUpdate(const Member& member,
                                           unsigned int incarnation) {
  const std::lock_guard<std::mutex> lock(mutex_members_);
  if (members_.find(member) != members_.end()) {
    if (members_[member] < incarnation) {
      members_[member] = incarnation;
      Notify();
    }
  } else {
    members_[member] = incarnation;
    Notify();
  }
}

void membership::Membership::MergeDownUpdate(const Member& member,
                                             unsigned int incarnation) {
  if (member == self_) {
    return;
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    if (members_.find(member) != members_.end()) {
      members_.erase(member);
      Notify();
    }
  }
}

int membership::Membership::GetRetransmitLimit() const {
  return retransmit_multiplier_ * static_cast<int>(ceil(log(members_.size())));
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
                                      uint16_t port) {
  Member host(node_name, ip_address, port);
  host_ = host;

  return 0;
}

int membership::Config::AddOneSeedMember(const std::string& node_name,
                                         const std::string& ip_address,
                                         uint16_t port) {
  Member seed(node_name, ip_address, port);

  // TODO(davidzwb): existing member check
  seed_members_.push_back(seed);

  return 0;
}

void membership::Config::AddRetransmitMultiplier(int multiplier) {
  retransmit_multiplier_ = multiplier;
}
