/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_EVENT_H_
#define NODE_KEEPER_SRC_EVENT_H_
#include <algorithm>
#include <vector>

#include "src/membership.h"

namespace node_keeper {
struct MemberEvent {
  enum Type { kMemberDown = 1, kMemberUp = 2 };
  Type type;
  membership::Member member;
};

class MemberEventGenerator {
 public:
  using Members = std::vector<membership::Member>;
  std::vector<MemberEvent> Update(const Members& members) {
    std::vector<MemberEvent> result;
    auto comparator = [](const membership::Member& lhs,
                         const membership::Member& rhs) {
      return lhs.GetIpAddress() < rhs.GetIpAddress();
    };
    std::vector<membership::Member> up;
    std::set_difference(members.begin(), members.end(), members_.begin(),
                        members_.end(), std::back_inserter(up), comparator);
    std::transform(up.begin(), up.end(), std::back_inserter(result),
                   [](const auto& member) {
                     return MemberEvent{MemberEvent::kMemberUp, member};
                   });

    std::vector<membership::Member> down;
    std::set_difference(members_.begin(), members_.end(), members.begin(),
                        members.end(), std::back_inserter(down), comparator);
    std::transform(down.begin(), down.end(), std::back_inserter(result),
                   [](const auto& member) {
                     return MemberEvent{MemberEvent::kMemberDown, member};
                   });
    members_ = members;
    return result;
  }

 private:
  Members members_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_EVENT_H_
