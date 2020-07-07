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
  enum Type {
    kMemberDown = 1,
    kMemberUp = 2,
    kActorSystemDown = 3,
    kActorSystemUp = 4
  };
  Type type;
  membership::Member member;
  std::vector<Actor> actors;
};

class MemberEventGenerator {
 public:
  using Members = std::vector<membership::Member>;
  using MemberActorSystems = std::map<membership::Member, bool>;

  std::vector<MemberEvent> Update(
      const Members& members, const MemberActorSystems& member_actor_systems) {
    std::vector<MemberEvent> result;
    auto member_events = generateEventForMember(members);
    result.insert(result.end(), member_events.begin(), member_events.end());

    auto actor_system_events =
        generateEventForActorSystem(member_actor_systems);
    result.insert(result.end(), actor_system_events.begin(),
                  actor_system_events.end());

    members_ = members;
    member_actor_systems_ = member_actor_systems;
    return result;
  }

  std::vector<MemberEvent> generateEventForActorSystem(
      const MemberActorSystems& member_actor_systems) {
    std::vector<MemberEvent> result;
    for (auto& [member, up] : member_actor_systems) {
      if (up != member_actor_systems_[member]) {
        result.push_back(MemberEvent{
            up ? MemberEvent::kActorSystemUp : MemberEvent::kActorSystemDown,
            member});
        continue;
      }
    }

    return result;
  }

  std::vector<MemberEvent> generateEventForMember(
      const Members& members) const {
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
    return result;
  }

 private:
  Members members_;
  MemberActorSystems member_actor_systems_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_EVENT_H_
