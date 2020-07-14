/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_MEMBERSHIP_H_
#define NODE_KEEPER_SRC_MEMBERSHIP_H_

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../../logger/include/logger.h"
#include "src/gossip.h"
#include "src/queue.h"

namespace membership {

enum ErrorCode {
  MEMBERSHIP_SUCCESS,
  MEMBERSHIP_FAILURE,
  MEMBERSHIP_INIT_HOSTMEMBER_EMPTY,
  MEMBERSHIP_INIT_TRANSPORT_EMPTY,
  MEMBERSHIP_CONFIG_IP_ADDRESS_INVALID
};

class Member {
 public:
  Member() : port_(0) {}
  Member(std::string node_name, std::string ip_address, uint16_t port,
         std::string host_name = "", std::string uid = "",
         std::string role = "")
      : node_name_(std::move(node_name)),
        host_name_(std::move(host_name)),
        ip_address_(std::move(ip_address)),
        port_(port),
        uid_(std::move(uid)),
        role_(std::move(role)) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);
  friend bool operator<(const Member& lhs, const Member& rhs);

  std::string GetNodeName() const { return node_name_; }
  std::string GetHostName() const { return host_name_; }
  std::string GetIpAddress() const { return ip_address_; }
  std::string GetRole() const { return role_; }
  uint16_t GetPort() const { return port_; }
  std::string GetUid() const { return uid_; }

  bool IsEmptyMember();

 private:
  std::string uid_;
  std::string node_name_;
  std::string host_name_;
  std::string ip_address_;
  std::string role_;
  uint16_t port_;
};

bool operator==(const Member& lhs, const Member& rhs);
bool operator!=(const Member& lhs, const Member& rhs);
bool operator<(const Member& lhs, const Member& rhs);

class Config {
 public:
  Config()
      : retransmit_multiplier_(3),
        gossip_interval_(500),
        failure_detector_interval_(2000),
        leave_without_notification_(false),
        failure_detector_off_(false),
        relay_ping_enabled_(false),
        logger_name_("default_logger") {}

  int SetHostMember(const std::string& node_name, const std::string& ip_address,
                    uint16_t port, std::string role = "");
  Member GetHostMember() const { return host_; }

  int AddOneSeedMember(const std::string& node_name,
                       const std::string& ip_address, uint16_t port);
  std::vector<Member> GetSeedMembers() const { return seed_members_; }

  /* Retransmit a message to RetransmitMultiplier * log(N+1) nodes, to ensure
     that messages are propagated through the entire cluster.*/
  void SetRetransmitMultiplier(int multiplier);
  int GetRetransmitMultiplier() const { return retransmit_multiplier_; }

  void SetGossipInterval(unsigned int interval);
  unsigned int GetGossipInterval() const { return gossip_interval_; }

  void SetFailureDetectorIntervalInMilliSeconds(unsigned int interval) {
    failure_detector_interval_ = interval;
  }

  unsigned int GetFailureDetectorIntervalInMilliSeconds() const {
    return failure_detector_interval_;
  }

  void EnableLeaveWithoutNotification() { leave_without_notification_ = true; }
  bool IsLeaveWithoutNotificationEnabled() const {
    return leave_without_notification_;
  }

  void SetFailureDetectorOff() { failure_detector_off_ = true; }
  bool IsFailureDetectorOff() const { return failure_detector_off_; }

  void EnableRelayPing() { relay_ping_enabled_ = true; }
  bool IsRelayPingEnabled() const { return relay_ping_enabled_; }

  void SetLoggerName(const std::string& logger_name) {
    logger_name_ = logger_name;
  }
  std::string GetLoggerName() const { return logger_name_; }

 private:
  Member host_;
  std::vector<Member> seed_members_;
  int retransmit_multiplier_;
  unsigned int gossip_interval_;
  unsigned int failure_detector_interval_;
  bool leave_without_notification_;
  bool failure_detector_off_;
  bool relay_ping_enabled_;
  std::string logger_name_;
};

class Subscriber {
 public:
  void virtual Update() = 0;
};

class Membership {
 public:
  Membership()
      : retransmit_multiplier_(3),
        incarnation_(0),
        if_notify_leave_(true),
        is_self_actor_system_up_(false) {}
  ~Membership();
  int Init(std::shared_ptr<gossip::Transportable> transport,
           const Config& config);
  std::vector<Member> GetMembers() const;
  std::vector<Member> GetSuspects() const;
  void Subscribe(std::shared_ptr<Subscriber> subscriber);
  void SendGossip(const gossip::Payload& payload);
  void NotifyActorSystemDown();
  Member GetSelf() const;
  int IncreaseIncarnation();
  std::map<Member, bool> GetActorSystems() const;
  void NotifyLeave();
  void SetSelfActorSystemUp();
  void SendSelfActorSystemUpGossip();

 private:
  int AddMember(const Member& member);
  int AddOrUpdateSuspect(const Member& member, unsigned int incarnation);
  bool IfBelongsToMembers(const membership::Member& member) const;
  bool IfBelongsToSuspects(const membership::Member& member) const;
  unsigned int GetMemberLocalIncarnation(const membership::Member& member);
  unsigned int GetSuspectLocalIncarnation(const membership::Member& member);
  void MergeUpUpdate(const Member& member, unsigned int incarnation);
  void MergeActorSystemDown(const Member& member, unsigned int incarnation);
  void MergeActorSystemUp(const Member& member, unsigned int incarnation);
  void MergeDownUpdate(const Member& member, unsigned int incarnation);
  void MergeMembers(const std::map<membership::Member, int>& members);
  void Notify();
  void HandleGossip(const struct gossip::Address& node,
                    const gossip::Payload& payload);
  void EraseExpiredMember(const membership::Member& member);
  gossip::Address GetRandomSeedAddress() const;
  std::pair<bool, membership::Member> GetRandomMember() const;
  std::pair<bool, membership::Member> GetRandomPingTarget() const;
  std::pair<bool, membership::Member> GetRelayMember(
      const std::set<Member>& exclude_members) const;
  std::vector<gossip::Address> GetAllMemberAddress();

  void DisseminateGossip(const gossip::Payload& payload);
  void PullFromSeedMember();
  void HandleDidPull(const gossip::Transportable::PullResult& result);
  std::vector<uint8_t> HandlePull(const gossip::Address& address,
                                  const void* data, size_t size);
  void HandlePush(const gossip::Address& address, const void* data,
                  size_t size);
  void Ping();
  void RelayPing(const Member& ping_target, std::set<Member> exclude_members);
  void Suspect(const Member& member, unsigned int incarnation);
  void RecoverySuspect(const Member& member);
  int GetRetransmitLimit() const;

  std::map<Member, int> members_;
  std::map<Member, int> suspects_;
  // Todo(davidzwb): consider using a read write lock instead
  mutable std::mutex mutex_members_;
  mutable std::mutex mutex_suspects_;
  std::map<Member, bool> member_actor_system_;
  mutable std::mutex mutex_member_actor_system_;
  Member self_;
  bool is_self_actor_system_up_;
  std::vector<Member> seed_members_;
  std::shared_ptr<gossip::Transportable> transport_;
  std::vector<std::shared_ptr<Subscriber>> subscribers_;
  // queue must be destroyed before transport i.e. put after transport otherwise
  // potential deadlock
  std::unique_ptr<queue::TimedFunctorQueue> gossip_queue_;
  std::unique_ptr<queue::TimedFunctorQueue> failure_detector_queue_;
  std::atomic_uint incarnation_;
  int retransmit_multiplier_;
  bool if_notify_leave_;
  bool is_relay_ping_enabled_;
  std::shared_ptr<cdcf::Logger> logger_;
};

};  // namespace membership

#endif  // NODE_KEEPER_SRC_MEMBERSHIP_H_
