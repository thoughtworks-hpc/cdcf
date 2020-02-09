//
// Created by Zhao Wenbo on 2020/1/19.
//

#ifndef CDCF_MEMBERSHIP_MESSAGE_H
#define CDCF_MEMBERSHIP_MESSAGE_H

#include <string>

#include "include/membership.h"
#include "message.pb.h"

namespace membership {

// TODO incarnation number presence

class Message {
 public:
  void InitAsUpMessage(const Member& member, unsigned int incarnation);
  void InitAsDownMessage(const Member& member, unsigned int incarnation);
  bool IsUpMessage();
  bool IsDownMessage();
  Member GetMember();

  unsigned int GetIncarnation() { return incarnation_; }

  std::string SerializeToString();
  void DeserializeFromString(const std::string& data);
  void DeserializeFromArray(const void* data, int size);

 private:
  MemberUpdate msg_;
  unsigned int incarnation_;
};

};      // namespace membership
#endif  // CDCF_MEMBERSHIP_MESSAGE_H
