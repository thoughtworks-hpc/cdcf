/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/membership.h"

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "src/membership_message.h"
#include "src/net_common.h"
#include "src/uuid.h"

membership::Membership::~Membership() {
  if (if_notify_leave_) {
    NotifyLeave();
  }
}

void membership::Membership::NotifyLeave() {
  if (this->members_.size() > 1) {
    membership::UpdateMessage message;
    message.InitAsDownMessage(this->self_, IncreaseIncarnation());
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

  CDCF_LOGGER_INFO("Node {} {}:{} initializing...", member.GetNodeName(),
                   member.GetIpAddress(), member.GetPort());
  print_ping_log_ = config.IsPrintPingLogEnable();
  self_ = member;
  AddMember(member);

  if (transport == nullptr) {
    CDCF_LOGGER_CRITICAL("No transport layer specified!");
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
    for (const auto& seed : config.GetSeedMembers()) {
      if (seed != self_) {
        seed_members_.push_back(seed);
      }
    }

    if (!seed_members_.empty()) {
      PullFromSeedMember();
    } else {
      CDCF_LOGGER_WARN("No valid seed provided");
    }
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
    CDCF_LOGGER_INFO("Try to pull from seed {}:{}", random_address.host,
                     random_address.port);
    transport_->Pull(random_address, pull_request_message.data(),
                     pull_request_message.size(),
                     [random_address,
                      this](const gossip::Transportable::PullResult& result) {
                       if (result.first == gossip::ErrorCode::kOK) {
                         CDCF_LOGGER_INFO("Pull from seed {}:{} succeeded",
                                          random_address.host,
                                          random_address.port);
                         HandleDidPull(result);
                         return;
                       }
                       std::this_thread::sleep_for(std::chrono::seconds(1));
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

  CDCF_LOGGER_INFO("Add new member {} {}:{}", member.GetNodeName(),
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

void membership::Membership::EraseExpiredMember(
    const membership::Member& member) {
  bool clear_old = false;
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);
    if (auto it = members_.find(member); it != members_.end()) {
      Member saved_member = it->first;
      if (saved_member.GetUid() != member.GetUid()) {
        members_.erase(saved_member);
        clear_old = true;
      }
    }
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_suspects_);
    if (auto it = suspects_.find(member); it != suspects_.end()) {
      Member saved_member = it->first;
      if (saved_member.GetUid() != member.GetUid()) {
        suspects_.erase(saved_member);
        clear_old = true;
      }
    }
  }
  if (clear_old) {
    {
      const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
      member_actor_system_[member] = false;
    }
    CDCF_LOGGER_INFO("clear old node, mark it's actor system down");
    Notify();
  }
}

void membership::Membership::HandleGossip(const struct gossip::Address& node,
                                          const gossip::Payload& payload) {
  membership::UpdateMessage message;
  message.DeserializeFromArray(payload.data.data(), payload.data.size());
  Member member = message.GetMember();

  if (message.IsUpMessage()) {
    CDCF_LOGGER_INFO(
        "Receive gossip up message for {}:{}, message incarnation={}, role={}",
        member.GetIpAddress(), member.GetPort(), message.GetIncarnation(),
        member.GetRole());
    if (member == self_) {
      CDCF_LOGGER_DEBUG("Ignore self gossip up message");
      return;
    }

    EraseExpiredMember(member);

    if (IfBelongsToSuspects(member) &&
        GetSuspectLocalIncarnation(member) >= message.GetIncarnation()) {
      return;
    }

    if (IfBelongsToMembers(member) &&
        GetMemberLocalIncarnation(member) >= message.GetIncarnation()) {
      return;
    }
    CDCF_LOGGER_DEBUG("Disseminate gossip, node up");
    gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                        GetRetransmitLimit());
    MergeUpUpdate(message.GetMember(), message.GetIncarnation());
  } else if (message.IsDownMessage()) {
    CDCF_LOGGER_INFO("Receive gossip down message for {}:{}",
                     member.GetIpAddress(), member.GetPort());
    if (!IfBelongsToMembers(member) && !IfBelongsToSuspects(member)) {
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
  } else if (message.IsActorSystemDownMessage()) {
    CDCF_LOGGER_INFO("Receive gossip actors system down message for {}:{}",
                     member.GetNodeName(), member.GetPort());
    if (!IfBelongsToMembers(member)) {
      return;
    }
    if (GetMemberLocalIncarnation(member) >= message.GetIncarnation()) {
      return;
    }
    SendGossip(payload);
    MergeActorSystemDown(member, message.GetIncarnation());
  } else if (message.IsActorSystemUpMessage()) {
    CDCF_LOGGER_INFO("Receive gossip actors system up message for {}:{}",
                     member.GetNodeName(), member.GetPort());
    if (!IfBelongsToMembers(member)) {
      return;
    }
    if (GetMemberLocalIncarnation(member) >= message.GetIncarnation()) {
      return;
    }
    SendGossip(payload);
    MergeActorSystemUp(member, message.GetIncarnation());
  }
}

void membership::Membership::HandleDidPull(
    const gossip::Transportable::PullResult& result) {
  if (result.first == gossip::ErrorCode::kOK) {
    FullStateMessage message;
    message.DeserializeFromArray(result.second.data(), result.second.size());

    if (message.IsSuccess()) {
      UpdateActorSystemStatus(message.GetMembersWithStatus());

      UpdateMessage update;
      auto incarnation = IncreaseIncarnation();
      update.InitAsUpMessage(self_, incarnation);
      CDCF_LOGGER_INFO(
          "Send self gossip up message to others, incarnation={}, role={}",
          incarnation, self_.GetRole());
      auto update_serialized = update.SerializeToString();
      gossip::Payload payload(update_serialized.data(),
                              update_serialized.size());

      gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                          GetRetransmitLimit());
      if (is_self_actor_system_up_) {
        SendSelfActorSystemUpGossip();
      }
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
    CDCF_LOGGER_INFO("Received full state pull request from {}:{}",
                     address.host, address.port);

    //    std::vector<Member> members;
    //    for (const auto& member : members_) {
    //      members.emplace_back(member.first.GetNodeName(),
    //                           member.first.GetIpAddress(),
    //                           member.first.GetPort(),
    //                           member.first.GetHostName(),
    //                           member.first.GetUid(), member.first.GetRole());
    //    }
    //    response.InitAsFullStateMessage(members);

    auto members_with_status = GetMembersWithStatus();
    response.InitAsFullStateMessageWithStatus(members_with_status);

    message_serialized = response.SerializeToString();
  } else if (request.IsPingType()) {
    if (print_ping_log_) {
      CDCF_LOGGER_DEBUG("Received ping request from {}:{}", address.host,
                        address.port);
    }
    PullResponseMessage response;
    response.InitAsPingSuccess(self_);
    message_serialized = response.SerializeToString();
    MergeMembers(request.GetMembersWithIncarnation(),
                 request.GetMembersWithActorSystem());
  } else if (request.IsPingRelayType()) {
    if (transport_) {
      if (print_ping_log_) {
        CDCF_LOGGER_DEBUG("Received ping relay request from {}:{}",
                          address.host, address.port);
      }

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
      std::map<Member, int> members;
      std::map<Member, bool> member_actor_system;
      {
        const std::lock_guard<std::mutex> lock(mutex_members_);
        members = members_;
      }
      {
        const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
        member_actor_system = member_actor_system_;
      }
      message.InitAsPingType(members, member_actor_system);

      std::string pull_request_message = message.SerializeToString();

      auto [get_success, random_member] = GetRandomPingTarget();
      if (!get_success) {
        Ping();
        return;
      }

      auto ping_target = random_member;
      gossip::Address address{ping_target.GetIpAddress(),
                              ping_target.GetPort()};
      if (print_ping_log_) {
        CDCF_LOGGER_DEBUG("Ping member {}:{} to check if it's alive",
                          ping_target.GetIpAddress(), ping_target.GetPort());
      }
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

    {
      const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
      member_actor_system_[member] = false;
    }

    CDCF_LOGGER_INFO("Start to suspect member {} {}:{}", member.GetNodeName(),
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
    CDCF_LOGGER_INFO("Add member {}:{} by merging up update",
                     member.GetIpAddress(), member.GetPort());
  }

  Notify();
}

void membership::Membership::UpdateActorSystemStatus(
    const std::vector<MemberWithStatus>& members_with_status) {
  const std::lock_guard<std::mutex> lock_members(mutex_members_);
  const std::lock_guard<std::mutex> lock_actor_system(
      mutex_member_actor_system_);

  for (const auto& member_with_status : members_with_status) {
    if (members_.find(member_with_status.member) == members_.end()) {
      members_[member_with_status.member] = 0;
      CDCF_LOGGER_INFO("Add member {}:{} by merging up update",
                       member_with_status.member.GetIpAddress(),
                       member_with_status.member.GetPort());
    }

    if (MemberUpdate::ACTOR_SYSTEM_UP == member_with_status.status) {
      member_actor_system_[member_with_status.member] = true;
      CDCF_LOGGER_INFO("Add up actor system {}:{} by merging up update",
                       member_with_status.member.GetIpAddress(),
                       member_with_status.member.GetPort());
    } else {
      member_actor_system_[member_with_status.member] = false;
    }
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
    if (members_.find(member) != members_.end()) {
      members_.erase(member);
      CDCF_LOGGER_INFO("Remove member {}:{} by merging down update",
                       member.GetIpAddress(), member.GetPort());
    }
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_suspects_);
    if (suspects_.find(member) != suspects_.end()) {
      suspects_.erase(member);
      CDCF_LOGGER_INFO("Remove suspect member {}:{} by merging down update",
                       member.GetIpAddress(), member.GetPort());
    }
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
    member_actor_system_[member] = false;
    CDCF_LOGGER_INFO("Set actor system down by merging node down update");
  }

  Notify();
}

void membership::Membership::MergeMembers(
    const std::map<membership::Member, int>& members,
    const std::map<membership::Member, bool>& member_actor_system) {
  bool should_notify = false;
  {
    const std::lock_guard<std::mutex> lock_members_(mutex_members_);
    const std::lock_guard<std::mutex> lock_member_actor_system(
        mutex_member_actor_system_);
    for (const auto& member_pair : members) {
      auto member = member_pair.first;
      int incarnation = member_pair.second;
      if (print_ping_log_) {
        CDCF_LOGGER_DEBUG("Receive update for ping member {}:{}",
                          member.GetIpAddress(), member.GetPort());
      }
      if (members_.find(member) != members_.end()) {
        if (incarnation > members_[member]) {
          members_[member] = incarnation;
          if (auto it = member_actor_system.find(member);
              it != member_actor_system.end()) {
            member_actor_system_[member] = it->second;
          }
          should_notify = true;
          CDCF_LOGGER_INFO(
              "Update member {}:{}'s incarnation by merging ping member",
              member.GetIpAddress(), member.GetPort());
        }
      } else {
        members_[member] = incarnation;
        if (auto it = member_actor_system.find(member);
            it != member_actor_system.end()) {
          member_actor_system_[member] = it->second;
        }
        should_notify = true;
        CDCF_LOGGER_INFO("Add member {}:{} by merging ping member",
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
void membership::Membership::SendGossip(const gossip::Payload& payload) {
  gossip_queue_->Push([this, payload]() { DisseminateGossip(payload); },
                      GetRetransmitLimit());
}
membership::Member membership::Membership::GetSelf() const { return self_; }

int membership::Membership::IncreaseIncarnation() { return ++incarnation_; }

void membership::Membership::MergeActorSystemUp(
    const membership::Member& member, unsigned int incarnation) {
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);

    if ((members_.find(member) != members_.end() &&
         members_[member] >= incarnation)) {
      return;
    }
    members_[member] = incarnation;
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
    member_actor_system_[member] = true;
    CDCF_LOGGER_INFO("merge actor system up success. member: {}:{}",
                     member.GetNodeName(), member.GetPort());
  }

  Notify();
}

void membership::Membership::MergeActorSystemDown(
    const membership::Member& member, unsigned int incarnation) {
  {
    const std::lock_guard<std::mutex> lock(mutex_members_);

    if ((members_.find(member) != members_.end() &&
         members_[member] >= incarnation)) {
      return;
    }
    members_[member] = incarnation;
  }

  {
    const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
    member_actor_system_[member] = false;
    CDCF_LOGGER_INFO("merge actor system down success. member: {}:{}",
                     member.GetNodeName(), member.GetPort());
  }

  Notify();
}

void membership::Membership::NotifyActorSystemDown() {
  membership::UpdateMessage message;
  is_self_actor_system_up_ = false;
  message.InitAsActorSystemDownMessage(self_, IncreaseIncarnation());
  auto serialized = message.SerializeToString();
  gossip::Payload payload(serialized.data(), serialized.size());
  SendGossip(payload);
  CDCF_LOGGER_INFO("send self actor system down gossip, incarnation: ",
                   message.GetIncarnation());
}
std::map<membership::Member, bool> membership::Membership::GetActorSystems()
    const {
  const std::lock_guard<std::mutex> lock(mutex_member_actor_system_);
  return member_actor_system_;
}
void membership::Membership::SetSelfActorSystemUp() {
  is_self_actor_system_up_ = true;
}
void membership::Membership::SendSelfActorSystemUpGossip() {
  membership::UpdateMessage message;
  message.InitAsActorSystemUpMessage(self_, IncreaseIncarnation());
  auto serialized = message.SerializeToString();
  gossip::Payload payload(serialized.data(), serialized.size());
  SendGossip(payload);
  CDCF_LOGGER_INFO(
      "send self actor system up gossip, Member: {}, Incarnation: {}",
      message.GetMember().GetNodeName(), message.GetIncarnation());
}

std::vector<membership::MemberWithStatus>
membership::Membership::GetMembersWithStatus() {
  std::vector<MemberWithStatus> members_with_status;
  std::vector<Member> return_members;
  const std::lock_guard<std::mutex> lock(mutex_members_);
  const std::lock_guard<std::mutex> actor_system_lock(
      mutex_member_actor_system_);

  for (auto& one_member_info : members_) {
    MemberWithStatus member_with_status;
    member_with_status.member = one_member_info.first;

    if (0 != member_actor_system_.count(member_with_status.member) &&
        member_actor_system_[member_with_status.member]) {
      member_with_status.status = MemberUpdate::ACTOR_SYSTEM_UP;
    } else {
      member_with_status.status = MemberUpdate::UP;
    }

    members_with_status.emplace_back(member_with_status);
  }

  return members_with_status;
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
                                      uint16_t port, std::string role) {
  std::string hostname;
  std::string ip_address_converted;
  auto ret =
      membership::ResolveHostName(ip_address, hostname, ip_address_converted);

  Member host(node_name, ip_address_converted, port, hostname,
              uuid::generate_uuid_v4(), role);
  CDCF_LOGGER_DEBUG("SetHostMember, role={}", host.GetRole());
  host_ = host;

  return ret;
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
