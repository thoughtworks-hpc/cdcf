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
    kActorsUp = 3,
    kActorSystemDown = 4
  };
  Type type;
  membership::Member member;
  std::vector<Actor> actors;
};

class MemberEventGenerator {
 public:
  using Members = std::vector<membership::Member>;
  using MemberActors = std::map<membership::Member, std::set<Actor>>;
  std::vector<MemberEvent> Update(const Members& members,
                                  const MemberActors& member_actors) {
    std::vector<MemberEvent> result;
    auto member_events = generateEventForMember(members);
    result.insert(result.end(), member_events.begin(), member_events.end());

    auto actor_events = generateEventForActors(members, member_actors);
    result.insert(result.end(), actor_events.begin(), actor_events.end());

    members_ = members;
    member_actors_ = member_actors;
    return result;
  }

  std::vector<MemberEvent> generateEventForActors(
      const Members& members, const MemberActors& member_actors) {
    std::vector<MemberEvent> result;
    for (auto& member : members) {
      std::vector<Actor> up;
      auto it = member_actors.find(member);
      if (it == member_actors.end()) {
        continue;
      }
      auto actors = it->second;
      auto& old_actors = member_actors_[member];
      std::set_difference(actors.begin(), actors.end(), old_actors.begin(),
                          old_actors.end(), std::back_inserter(up));

      if (up.size() > 0) {
        result.push_back(MemberEvent{MemberEvent::kActorsUp, member, up});
      }

      if (old_actors.size() > 0 && actors.size() == 0) {
        result.push_back(MemberEvent{MemberEvent::kActorSystemDown, member});
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
  MemberActors member_actors_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_EVENT_H_
