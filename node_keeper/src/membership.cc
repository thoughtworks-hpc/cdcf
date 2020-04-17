/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/membership.h"

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "src/membership_message.h"

membership::Membership::~Membership() {
  if (if_notify_leave_) {
    NotifyLeave();
  }
}

void membership::Membership::NotifyLeave() {
  if (this->members_.size() > 1) {
    membership::UpdateMessage message;
    message.InitAsDownMessage(this->self_, this->incarnation_ + 1);
    auto message_serialized = message.SerializeToString();
    gossip::Payload payload{message_serialized};

    int retransmit_limit = this->GetRetransmitLimit();
    while (retransmit_limit > 0) {
      this->transport_->Gossip(this->GetAllMemberAddress(), payload);
      retransmit_limit--;
    }
  }
}

std::vector<membership::Member> membership::Membership::GetMembers() const {
  std::vector<Member> return_members;
  const std::lock_guard<std::mutex> lock(mutex_members_);
  for (auto& member : members_) {
    return_members.push_back(member.first);
  }
  return return_members;
}

std::vector<membership::Member> membership::Membership::GetSuspects() const {
  std::vector<Member> return_members;
  const std::lock_guard<std::mutex> lock(mutex_suspects_);
  for (auto& member : suspects_) {
    return_members.push_back(member.first);
  }
  return return_members;
}

int membership::Membership::Init(
    std::shared_ptr<gossip::Transportable> transport, const Config& config) {
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

  if_notify_leave_ = !config.IfLeaveWithoutNotification();

  gossip_queue_ = std::make_unique<queue::TimedFunctorQueue>(
      std::chrono::milliseconds(config.GetGossipInterval()));

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

  if (!config.IsFailureDetectorOff()) {
    failure_detector__queue_ =
        std::make_unique<queue::TimedFunctorQueue>(std::chrono::milliseconds(
            config.GetFailureDetectorIntervalInMilliSeconds()));
    Ping();
  }

  return MEMBERSHIP_SUCCESS;
}

void membership::Membership::PullFromSeedMember() {
  std::string pull_request_message = "pull";

  if (transport_) {
    transport_->Pull(GetRandomSeedAddress(), pull_request_message.data(),
                     pull_request_message.size(),
                     [this](const gossip::Transportable::PullResult& result) {
                       if (result.first == gossip::ErrorCode::kOK) {
                         HandleDidPull(result);
                         return;
                       }
                       PullFromSeedMember();
                     });
  }
}

gossip::Address membership::Membership::GetRandomSeedAddress() const {
  static std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, this->seed_members_.size() - 1);

  auto peer = this->seed_members_[dis(gen)];
  return gossip::Address{peer.GetIpAddress(), peer.GetPort()};
}

std::pair<bool, membership::Member> membership::Membership::GetRandomMember()
    const {
  std::vector<Member> members_without_self;
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    if (members_.size() <= 1) {
      return std::make_pair(false, Member());
    }

    for (const auto& member_pair : members_) {
      if (member_pair.first != self_) {
        members_without_self.push_back(member_pair.first);
      }
    }
  }

  static std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, members_without_self.size() - 1);

  return std::make_pair(true, members_without_self[dis(gen)]);
}

int membership::Membership::AddMember(const membership::Member& member) {
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    members_[member] = incarnation_;
  }
  Notify();

  return 0;
}

int membership::Membership::AddOrUpdateSuspect(const membership::Member& member,
                                               unsigned int incarnation) {
  {
    const std::lock_guard<std::mutex> lock(mutex_suspects_);
    suspects_[member] = incarnation;
  }

  return 0;
}

std::vector<gossip::Address> membership::Membership::GetAllMemberAddress() {
  std::vector<gossip::Address> addresses;
  const std::lock_guard<std::mutex> lock(mutex_members_);
  for (const auto& member : members_) {
    if (member.first != self_) {
      addresses.emplace_back(
          gossip::Address{member.first.GetIpAddress(), member.first.GetPort()});
    }
  }

  return addresses;
}

void membership::Membership::HandleGossip(const struct gossip::Address& node,
                                          const gossip::Payload& payload) {
  membership::UpdateMessage message;
  message.DeserializeFromArray(payload.data.data(), payload.data.size());
  Member member = message.GetMember();

  if (message.IsUpMessage()) {
    // Ignore left members
    if (left_members_.find(member) != left_members_.end()) {
      return;
    }

    if (IfBelongsToSuspects(member) &&
        GetSuspectLocalIncarnation(member) >= message.GetIncarnation()) {
      return;
    }

    if (IfBelongsToMembers(member) &&
        GetMemberLocalIncarnation(member) == message.GetIncarnation()) {
      return;
    }

    gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                        GetRetransmitLimit());
    MergeUpUpdate(message.GetMember(), message.GetIncarnation());
  } else if (message.IsDownMessage()) {
    if (!IfBelongsToMembers(member)) {
      return;
    }

    gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                        GetRetransmitLimit());
    MergeDownUpdate(message.GetMember(), message.GetIncarnation());
  } else if (message.IsSuspectMessage()) {
    if (IfBelongsToSuspects(member)) {
      if (GetSuspectLocalIncarnation(member) < message.GetIncarnation()) {
        AddOrUpdateSuspect(member, message.GetIncarnation());
      }
      return;
    }

    if (IfBelongsToMembers(member) &&
        message.GetIncarnation() >= GetMemberLocalIncarnation(member)) {
      Suspect(member, message.GetIncarnation());
    }
  }
}

void membership::Membership::HandleDidPull(
    const gossip::Transportable::PullResult& result) {
  if (result.first == gossip::ErrorCode::kOK) {
    FullStateMessage message;
    message.DeserializeFromArray(result.second.data(), result.second.size());

    if (message.IsSuccess()) {
      for (const auto& member : message.GetMembers()) {
        MergeUpUpdate(member, 0);
      }

      UpdateMessage update;
      update.InitAsUpMessage(self_, incarnation_);
      auto update_serialized = update.SerializeToString();
      gossip::Payload payload(update_serialized.data(),
                              update_serialized.size());

      // DisseminateGossip(payload);
      gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                          GetRetransmitLimit());
    } else {
      // Rejected from reentry
    }
  }
}

void membership::Membership::DisseminateGossip(const gossip::Payload& payload) {
  auto did_gossip = [](gossip::ErrorCode error) {};
  auto pair = GetRandomMember();

  if (!pair.first) {
    return;
  }
  auto member = pair.second;

  std::vector<gossip::Address> address{
      {member.GetIpAddress(), member.GetPort()}};

  if (transport_) {
    transport_->Gossip(address, payload, did_gossip);
  }
}

bool membership::Membership::IsLeftMember(const gossip::Address& address) {
  for (auto member : left_members_) {
    if (member.GetPort() == address.port &&
        member.GetIpAddress() == address.host) {
      return true;
    }
  }
  return false;
}

std::vector<uint8_t> membership::Membership::HandlePull(
    const gossip::Address& address, const void* data, size_t size) {
  FullStateMessage message;

  if (IsLeftMember(address)) {
    message.InitAsReentryRejected();
  } else {
    std::vector<Member> members;
    for (const auto& member : members_) {
      members.emplace_back(member.first.GetNodeName(),
                           member.first.GetIpAddress(), member.first.GetPort());
    }
    message.InitAsFullStateMessage(members);
  }

  auto message_serialized = message.SerializeToString();

  return std::vector<uint8_t>(message_serialized.begin(),
                              message_serialized.end());
}

void membership::Membership::Ping() {
  auto failure_detector_functor = [this]() {
    if (transport_) {
      std::string pull_request_message = "ping";

      auto pair = GetRandomMember();
      if (!pair.first) {
        Ping();
        return;
      }

      auto member = pair.second;
      gossip::Address address{member.GetIpAddress(), member.GetPort()};

      transport_->Pull(
          address, pull_request_message.data(), pull_request_message.size(),
          [this, member](const gossip::Transportable::PullResult& result) {
            if (result.first != gossip::ErrorCode::kOK) {
              if (IfBelongsToMembers(member)) {
                Suspect(member, GetMemberLocalIncarnation(member));
              }
            }
          });
    }
    Ping();
  };

  if (failure_detector__queue_) {
    failure_detector__queue_->Push(failure_detector_functor, 1);
  }
}

void membership::Membership::Suspect(const Member& member,
                                     unsigned int incarnation) {
  if (IfBelongsToMembers(member)) {
    left_members_.insert(member);

    AddOrUpdateSuspect(member, incarnation);

    UpdateMessage update;
    update.InitAsSuspectMessage(member, incarnation);
    auto update_serialized = update.SerializeToString();
    gossip::Payload payload(update_serialized.data(), update_serialized.size());

    {
      const std::lock_guard<std::mutex> lock(mutex_members_);
      members_.erase(member);
    }

    gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                        GetRetransmitLimit());

    Notify();
  }
}

bool membership::Membership::IfBelongsToMembers(
    const membership::Member& member) const {
  const std::lock_guard<std::mutex> lock(mutex_members_);
  return members_.find(member) != members_.end();
}

bool membership::Membership::IfBelongsToSuspects(
    const membership::Member& member) const {
  const std::lock_guard<std::mutex> lock(mutex_suspects_);
  return suspects_.find(member) != suspects_.end();
}

unsigned int membership::Membership::GetMemberLocalIncarnation(
    const membership::Member& member) {
  const std::lock_guard<std::mutex> lock(mutex_members_);

  if (members_.find(member) == members_.end()) {
    return 0;
  }

  return members_[member];
}

unsigned int membership::Membership::GetSuspectLocalIncarnation(
    const membership::Member& member) {
  const std::lock_guard<std::mutex> lock(mutex_suspects_);

  if (suspects_.find(member) == suspects_.end()) {
    return 0;
  }

  return suspects_[member];
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
  if (left_members_.find(member) != left_members_.end()) {
    return;
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_members_);

    if ((members_.find(member) != members_.end() &&
         members_[member] >= incarnation)) {
      return;
    }

    members_[member] = incarnation;
  }

  Notify();
}

void membership::Membership::MergeDownUpdate(const Member& member,
                                             unsigned int incarnation) {
  if (member == self_) {
    return;
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    if (members_.find(member) == members_.end()) {
      return;
    }
    left_members_.insert(member);
    members_.erase(member);
  }

  Notify();
}

int membership::Membership::GetRetransmitLimit() const {
  const std::lock_guard<std::mutex> lock(mutex_members_);
  return retransmit_multiplier_ *
         static_cast<int>(ceil(log10(members_.size())));
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

bool membership::operator<(const membership::Member& lhs,
                           const membership::Member& rhs) {
  return (lhs.ip_address_ < rhs.ip_address_) ||
         (lhs.ip_address_ == rhs.ip_address_ && lhs.port_ < rhs.port_);
}

bool membership::Member::IsEmptyMember() {
  return node_name_.empty() && ip_address_.empty() && port_ == 0;
}

int membership::Config::AddHostMember(const std::string& node_name,
                                      const std::string& ip_address,
                                      uint16_t port) {
  Member host(node_name, ip_address, port);
  host_ = host;

  return MEMBERSHIP_SUCCESS;
}

int membership::Config::AddOneSeedMember(const std::string& node_name,
                                         const std::string& ip_address,
                                         uint16_t port) {
  Member seed(node_name, ip_address, port);

  for (const auto& member : seed_members_) {
    if (member == seed) {
      return MEMBERSHIP_FAILURE;
    }
  }

  seed_members_.push_back(seed);

  return MEMBERSHIP_SUCCESS;
}

void membership::Config::AddRetransmitMultiplier(int multiplier) {
  retransmit_multiplier_ = multiplier;
}

void membership::Config::AddGossipInterval(unsigned int interval) {
  gossip_interval_ = interval;
}
