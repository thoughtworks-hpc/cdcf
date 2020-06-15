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

  logger_ = std::make_shared<cdcf::Logger>(config.GetLoggerName());
  logger_->Info("Node {} {}:{} initializing...", member.GetNodeName(),
                member.GetIpAddress(), member.GetPort());

  self_ = member;
  AddMember(member);

  if (transport == nullptr) {
    logger_->Critical("No transport layer specified!");
    return MEMBERSHIP_INIT_TRANSPORT_EMPTY;
  }

  transport_ = transport;

  if_notify_leave_ = !config.IsLeaveWithoutNotificationEnabled();

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

  auto push_handler = [this](const gossip::Address& address, const void* data,
                             size_t size) { HandlePush(address, data, size); };
  transport->RegisterPushHandler(push_handler);

  retransmit_multiplier_ = config.GetRetransmitMultiplier();

  if (!config.GetSeedMembers().empty()) {
    seed_members_ = config.GetSeedMembers();
    PullFromSeedMember();
  }

  is_relay_ping_enabled_ = config.IsRelayPingEnabled();
  if (!config.IsFailureDetectorOff()) {
    failure_detector_queue_ =
        std::make_unique<queue::TimedFunctorQueue>(std::chrono::milliseconds(
            config.GetFailureDetectorIntervalInMilliSeconds()));
    Ping();
  }

  return MEMBERSHIP_SUCCESS;
}

void membership::Membership::PullFromSeedMember() {
  PullRequestMessage message;
  message.InitAsFullStateType();
  std::string pull_request_message = message.SerializeToString();

  if (transport_) {
    auto random_address = GetRandomSeedAddress();
    logger_->Info("Try to pull from seed {}:{}", random_address.host,
                  random_address.port);
    transport_->Pull(random_address, pull_request_message.data(),
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

// TODO(Yujia.Li): performance can be improve
std::pair<bool, membership::Member>
membership::Membership::GetRandomPingTarget() const {
  std::vector<Member> members_without_self;
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    const std::lock_guard<std::mutex> lock_suspect(mutex_suspects_);

    if (members_.size() + suspects_.size() <= 1) {
      return std::make_pair(false, Member());
    }

    for (const auto& [member, _] : members_) {
      if (member != self_) {
        members_without_self.push_back(member);
      }
    }

    for (const auto& [member, _] : suspects_) {
      members_without_self.push_back(member);
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

  logger_->Info("Add new member {} {}:{}", member.GetNodeName(),
                member.GetIpAddress(), member.GetPort());

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
    logger_->Info("Receive gossip up message for {}:{}", member.GetIpAddress(),
                  member.GetPort());
    // TODO(Yujia.Li): member's address here would be `HOST` in config

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
    logger_->Info("Receive gossip down message for {}:{}",
                  member.GetIpAddress(), member.GetPort());
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
  } else if (message.IsRecoveryMessage()) {
    if (!IfBelongsToSuspects(member)) {
      return;
    }
    if (message.GetIncarnation() < GetSuspectLocalIncarnation(member)) {
      return;
    }
    RecoverySuspect(member);
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

std::vector<uint8_t> membership::Membership::HandlePull(
    const gossip::Address& address, const void* data, size_t size) {
  std::string message_serialized = "pull";
  PullRequestMessage request;
  request.DeserializeFromArray(data, size);
  if (request.IsFullStateType()) {
    FullStateMessage response;

    // TODO(Yujia.Li): message's IP here would be IPv4
    std::vector<Member> members;
    for (const auto& member : members_) {
      members.emplace_back(member.first.GetNodeName(),
                           member.first.GetIpAddress(), member.first.GetPort());
    }
    response.InitAsFullStateMessage(members);

    message_serialized = response.SerializeToString();
  } else if (request.IsPingType()) {
    PullResponseMessage response;
    response.InitAsPingSuccess(self_);
    message_serialized = response.SerializeToString();
    MergeMembers(request.GetMembersWithIncarnation());
  } else if (request.IsPingRelayType()) {
    if (transport_) {
      PullRequestMessage request_message;
      request_message.InitAsPingType();
      std::string pull_request_message = request_message.SerializeToString();

      gossip::Address ping_target_address{
          request.GetIpAddress(), static_cast<uint16_t>(request.GetPort())};
      Member ping_target_member{request.GetName(), request.GetIpAddress(),
                                static_cast<uint16_t>(request.GetPort())};
      gossip::Address ping_result_destination_address{
          request.GetSelfIpAddress(),
          static_cast<uint16_t>(request.GetSelfPort())};

      auto did_pull = [ping_result_destination_address, ping_target_member,
                       this](const gossip::Transportable::PullResult& result) {
        std::string message_serialized;
        PullResponseMessage response_message;
        if (result.first == gossip::ErrorCode::kOK) {
          response_message.InitAsPingSuccess(ping_target_member);
        } else {
          response_message.InitAsPingFailure(ping_target_member);
        }
        message_serialized = response_message.SerializeToString();
        auto did_push = [](gossip::ErrorCode error) {};
        transport_->Push(ping_result_destination_address,
                         message_serialized.data(), message_serialized.size(),
                         did_push);
      };

      auto pull_result =
          transport_->Pull(ping_target_address, pull_request_message.data(),
                           pull_request_message.size(), did_pull);

      PullResponseMessage response_message;
      response_message.InitAsPingReceived();

      message_serialized = response_message.SerializeToString();
    }
  }

  return std::vector<uint8_t>(message_serialized.begin(),
                              message_serialized.end());
}

void membership::Membership::HandlePush(const gossip::Address& address,
                                        const void* data, size_t size) {
  PullResponseMessage message;
  message.DeserializeFromArray(data, size);
  Member ping_target = message.GetMember();

  if (message.IsPingFailure()) {
    Suspect(ping_target, GetMemberLocalIncarnation(ping_target));
  }
}

void membership::Membership::Ping() {
  auto failure_detector_functor = [this]() {
    if (transport_) {
      PullRequestMessage message;
      message.InitAsPingType(members_);
      std::string pull_request_message = message.SerializeToString();

      auto [get_success, random_member] = GetRandomPingTarget();
      if (!get_success) {
        Ping();
        return;
      }

      auto ping_target = random_member;
      gossip::Address address{ping_target.GetIpAddress(),
                              ping_target.GetPort()};
      logger_->Debug("Ping member {}:{} to check if it's alive",
                     ping_target.GetIpAddress(), ping_target.GetPort());
      transport_->Pull(
          address, pull_request_message.data(), pull_request_message.size(),
          [this, ping_target](const gossip::Transportable::PullResult& result) {
            if (result.first == gossip::ErrorCode::kOK &&
                IfBelongsToSuspects(ping_target)) {
              RecoverySuspect(ping_target);
              return;
            }

            if (result.first != gossip::ErrorCode::kOK &&
                IfBelongsToMembers(ping_target)) {
              if (is_relay_ping_enabled_) {
                std::set<Member> exclude_members;
                exclude_members.insert(self_);
                exclude_members.insert(ping_target);
                RelayPing(ping_target, exclude_members);
              } else {
                Suspect(ping_target, GetMemberLocalIncarnation(ping_target));
              }
            }
          });
    }
    Ping();
  };

  if (failure_detector_queue_) {
    failure_detector_queue_->Push(failure_detector_functor, 1);
  }
}

void membership::Membership::RelayPing(const membership::Member& ping_target,
                                       std::set<Member> exclude_members) {
  auto pair = GetRelayMember(exclude_members);
  bool if_get_relay_member_success = pair.first;

  if (if_get_relay_member_success) {
    PullRequestMessage message;
    Member relay_target = pair.second;
    gossip::Address relay_target_address{relay_target.GetIpAddress(),
                                         relay_target.GetPort()};
    message.InitAsPingRelayType(self_, ping_target);
    auto payload = message.SerializeToString();

    auto did_pull =
        [this, ping_target, relay_target, exclude_members](
            const gossip::Transportable::PullResult& result) mutable {
          if (result.first != gossip::ErrorCode::kOK) {
            exclude_members.insert(relay_target);
            RelayPing(ping_target, exclude_members);
          }
        };

    transport_->Pull(relay_target_address, payload.data(), payload.size(),
                     did_pull);
  } else {
    Suspect(ping_target, GetMemberLocalIncarnation(ping_target));
  }
}

void membership::Membership::Suspect(const Member& member,
                                     unsigned int incarnation) {
  if (IfBelongsToMembers(member)) {
    AddOrUpdateSuspect(member, incarnation);

    UpdateMessage update;
    update.InitAsSuspectMessage(member, incarnation);
    auto update_serialized = update.SerializeToString();
    gossip::Payload payload(update_serialized.data(), update_serialized.size());

    {
      const std::lock_guard<std::mutex> lock(mutex_members_);
      members_.erase(member);
    }
    logger_->Info("Start to suspect member {} {}:{}", member.GetNodeName(),
                  member.GetIpAddress(), member.GetPort());
    gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                        GetRetransmitLimit());

    Notify();
  }
}

std::pair<bool, membership::Member> membership::Membership::GetRelayMember(
    const std::set<Member>& exclude_members) const {
  if (members_.size() <= 2) {
    return std::make_pair(false, Member());
  }

  Member relay_to_member;
  bool found = false;

  for (const auto& member_pair : members_) {
    auto member = member_pair.first;
    if (exclude_members.find(member) == exclude_members.end()) {
      relay_to_member = member;
      found = true;
    }
  }

  return std::make_pair(found, relay_to_member);
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
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);

    if ((members_.find(member) != members_.end() &&
         members_[member] >= incarnation)) {
      return;
    }

    members_[member] = incarnation;
    logger_->Info("Add member {}:{} by merging up update",
                  member.GetIpAddress(), member.GetPort());
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
    members_.erase(member);
    logger_->Info("Remove member {}:{} by merging down update",
                  member.GetIpAddress(), member.GetPort());
  }

  Notify();
}

void membership::Membership::MergeMembers(
    const std::map<membership::Member, int>& members) {
  bool should_notify = false;
  {
    const std::lock_guard<std::mutex> lock_members_(mutex_members_);
    for (const auto& member_pair : members) {
      auto member = member_pair.first;
      int incarnation = member_pair.second;
      logger_->Debug("Receive update for ping member {}:{}",
                     member.GetIpAddress(), member.GetPort());
      if (members_.find(member) != members_.end()) {
        if (incarnation > members_[member]) {
          members_[member] = incarnation;
          should_notify = true;
          logger_->Info(
              "Update member {}:{}'s incarnation by merging ping member",
              member.GetIpAddress(), member.GetPort());
        }
      } else {
        members_[member] = incarnation;
        should_notify = true;
        logger_->Info("Add member {}:{} by merging ping member",
                      member.GetIpAddress(), member.GetPort());
      }
    }
  }
  if (should_notify) {
    Notify();
  }
}

int membership::Membership::GetRetransmitLimit() const {
  const std::lock_guard<std::mutex> lock(mutex_members_);
  return retransmit_multiplier_ *
         static_cast<int>(ceil(log10(members_.size())));
}
void membership::Membership::RecoverySuspect(const membership::Member& member) {
  int incarnation;
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    members_[member] = suspects_[member];
    incarnation = members_[member];
  }
  {
    const std::lock_guard<std::mutex> lock(mutex_suspects_);
    suspects_.erase(member);
  }

  UpdateMessage message;
  message.InitAsRecoveryMessage(member, incarnation);
  auto serialized = message.SerializeToString();
  gossip::Payload payload(serialized.data(), serialized.size());

  gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                      GetRetransmitLimit());

  Notify();
}

bool membership::operator==(const membership::Member& lhs,
                            const membership::Member& rhs) {
  return lhs.GetIpAddress() == rhs.GetIpAddress() &&
         lhs.GetPort() == rhs.GetPort();
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

int membership::Config::SetHostMember(const std::string& node_name,
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

void membership::Config::SetRetransmitMultiplier(int multiplier) {
  retransmit_multiplier_ = multiplier;
}

void membership::Config::SetGossipInterval(unsigned int interval) {
  gossip_interval_ = interval;
}
