/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef CDCF_MEMBERSHIP_MESSAGE_H
#define CDCF_MEMBERSHIP_MESSAGE_H

#include <string>

#include "include/membership.h"
#include "message.pb.h"

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
  bool IsUpMessage() const;
  bool IsDownMessage() const;
  Member GetMember() const;
  unsigned int GetIncarnation() const { return incarnation_; }

  google::protobuf::Message& BaseMessage() override { return update_; }

 private:
  MemberUpdate update_;
  unsigned int incarnation_;
};

class FullStateMessage : public Message {
 public:
  void InitAsFullStateMessage(const std::vector<Member>& members);
  std::vector<Member> GetMembers();

  google::protobuf::Message& BaseMessage() override { return state_; }

 private:
  MemberFullState state_;
};

};      // namespace membership
#endif  // CDCF_MEMBERSHIP_MESSAGE_H
