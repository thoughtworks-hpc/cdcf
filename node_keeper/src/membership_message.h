/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_MEMBERSHIP_MESSAGE_H_
#define NODE_KEEPER_SRC_MEMBERSHIP_MESSAGE_H_

#include <map>
#include <string>
#include <vector>

#include "protobuf/message.pb.h"
#include "src/membership.h"

namespace membership {

class Message {
 public:
  virtual google::protobuf::Message& BaseMessage() = 0;

  std::string SerializeToString();
  void DeserializeFromString(const std::string& data);
  void DeserializeFromArray(const void* data, int size);
};

class UpdateMessage : public Message {
 public:
  void InitAsUpMessage(const Member& member, unsigned int incarnation);
  void InitAsDownMessage(const Member& member, unsigned int incarnation);
  void InitAsSuspectMessage(const Member& member, unsigned int incarnation);
  void InitAsRecoveryMessage(const Member& member, unsigned int incarnation);
  void InitAsActorsUpMessage(const Member& member, unsigned int incarnation,
                             const std::vector<node_keeper::Actor>& actors);
  bool IsUpMessage() const;
  bool IsDownMessage() const;
  bool IsSuspectMessage() const;
  bool IsRecoveryMessage() const;
  bool IsActorsUpMessage() const;
  Member GetMember() const;

  unsigned int GetIncarnation() const { return update_.incarnation(); }

  std::vector<node_keeper::Actor> GetActors() const;

  google::protobuf::Message& BaseMessage() override { return update_; }

 private:
  MemberUpdate update_;
  unsigned int incarnation_;
};

class FullStateMessage : public Message {
 public:
  void InitAsFullStateMessage(const std::vector<Member>& members);
  void InitAsReentryRejected();
  bool IsSuccess();
  std::vector<Member> GetMembers();

  google::protobuf::Message& BaseMessage() override { return state_; }

 private:
  MemberFullState state_;
};

class PullRequestMessage : public Message {
 public:
  void InitAsFullStateType();
  void InitAsPingType();
  void InitAsPingType(const std::map<Member, int>& members);
  void InitAsPingRelayType(const Member& self, const Member& target);
  bool IsFullStateType();
  bool IsPingType();
  bool IsPingRelayType();
  std::string GetName();
  std::string GetIpAddress();
  unsigned int GetPort();
  std::string GetSelfIpAddress();
  unsigned int GetSelfPort();

  std::map<membership::Member, int> GetMembersWithIncarnation();

  google::protobuf::Message& BaseMessage() override { return pull_request_; }

 private:
  PullRequest pull_request_;
};

class PullResponseMessage : public Message {
 public:
  void InitAsPingSuccess(const Member& member);
  void InitAsPingFailure(const Member& member);
  void InitAsPingReceived();
  Member GetMember();
  bool IsPingSuccess();
  bool IsPingFailure();

  google::protobuf::Message& BaseMessage() override { return pull_response_; }

 private:
  PullResponse pull_response_;
};

};      // namespace membership
#endif  // NODE_KEEPER_SRC_MEMBERSHIP_MESSAGE_H_
