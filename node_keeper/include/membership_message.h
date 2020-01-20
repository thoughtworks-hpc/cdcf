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
  void InitAsUpMessage(const Member& member);
  bool IsUpMessage();
  Member GetMember();

  std::string SerializeToString();
  void DeserializeFromString(const std::string& data);

 private:
  MemberUpdate msg_;
};

};      // namespace membership
#endif  // CDCF_MEMBERSHIP_MESSAGE_H
