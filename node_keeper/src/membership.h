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

#include "src/gossip.h"
#include "src/queue.h"

namespace membership {

enum ErrorCode {
  MEMBERSHIP_SUCCESS,
  MEMBERSHIP_FAILURE,
  MEMBERSHIP_INIT_HOSTMEMBER_EMPTY,
  MEMBERSHIP_INIT_TRANSPORT_EMPTY
};

class Member {
 public:
  Member() : port_(0) {}
  Member(std::string node_name, std::string ip_address, uint16_t port)
      : node_name_(std::move(node_name)),
        ip_address_(std::move(ip_address)),
        port_(port) {}

  friend bool operator==(const Member& lhs, const Member& rhs);
  friend bool operator!=(const Member& lhs, const Member& rhs);
  friend bool operator<(const Member& lhs, const Member& rhs);

  std::string GetNodeName() const { return node_name_; }
  std::string GetIpAddress() const { return ip_address_; }
  uint16_t GetPort() const { return port_; }

  bool IsEmptyMember();

 private:
  std::string node_name_;
  std::string ip_address_;
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
        failure_detector_interval_(1000),
        leave_without_notification_(false),
        failure_detector_off_(false) {}

  int SetHostMember(const std::string& node_name, const std::string& ip_address,
                    uint16_t port);
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

 private:
  Member host_;
  std::vector<Member> seed_members_;
  int retransmit_multiplier_;
  unsigned int gossip_interval_;
  unsigned int failure_detector_interval_;
  bool leave_without_notification_;
  bool failure_detector_off_;
};

class Subscriber {
 public:
  void virtual Update() = 0;
};

class Membership {
 public:
  Membership()
      : retransmit_multiplier_(3), incarnation_(0), if_notify_leave_(true) {}
  ~Membership();
  int Init(std::shared_ptr<gossip::Transportable> transport,
           const Config& config);
  std::vector<Member> GetMembers() const;
  std::vector<Member> GetSuspects() const;
  void Subscribe(std::shared_ptr<Subscriber> subscriber);

 private:
  int AddMember(const Member& member);
  int AddOrUpdateSuspect(const Member& member, unsigned int incarnation);
  bool IfBelongsToMembers(const membership::Member& member) const;
  bool IfBelongsToSuspects(const membership::Member& member) const;
  unsigned int GetMemberLocalIncarnation(const membership::Member& member);
  unsigned int GetSuspectLocalIncarnation(const membership::Member& member);
  void MergeUpUpdate(const Member& member, unsigned int incarnation);
  void MergeDownUpdate(const Member& member, unsigned int incarnation);
  void Notify();
  void HandleGossip(const struct gossip::Address& node,
                    const gossip::Payload& payload);
  gossip::Address GetRandomSeedAddress() const;
  std::pair<bool, membership::Member> GetRandomMember() const;
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
  bool IsLeftMember(const gossip::Address& address);
  int GetRetransmitLimit() const;

  void NotifyLeave();

  std::map<Member, int> members_;
  std::map<Member, int> suspects_;
  mutable std::mutex mutex_members_;
  mutable std::mutex mutex_suspects_;
  Member self_;
  std::vector<Member> seed_members_;
  std::set<Member> left_members_;
  std::shared_ptr<gossip::Transportable> transport_;
  std::vector<std::shared_ptr<Subscriber>> subscribers_;
  // queue must be destroyed before transport i.e. put after transport otherwise
  // potential deadlock
  std::unique_ptr<queue::TimedFunctorQueue> gossip_queue_;
  std::unique_ptr<queue::TimedFunctorQueue> failure_detector_queue_;
  unsigned int incarnation_;
  int retransmit_multiplier_;
  bool if_notify_leave_;
};

};  // namespace membership

#endif  // NODE_KEEPER_SRC_MEMBERSHIP_H_
