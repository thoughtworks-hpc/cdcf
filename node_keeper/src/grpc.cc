/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

#include "membership_message.h"

namespace {
::Member& UpdateMember(::Member* to, const membership::Member& from) {
  to->set_name(from.GetNodeName());
  to->set_hostname(from.GetHostName());
  to->set_host(from.GetIpAddress());
  to->set_port(from.GetPort());
  return *to;
}
}  // namespace

namespace node_keeper {
::grpc::Status GRPCImpl::GetMembers(::grpc::ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    ::GetMembersReply* response) {
  for (auto& member : members_) {
    UpdateMember(response->add_members(), member);
  }
  return ::grpc::Status::OK;
}

::grpc::Status GRPCImpl::PushActorsUpInfo(::grpc::ServerContext* context,
                                          const ::ActorsUpInfo* request,
                                          ::google::protobuf::Empty* response) {
  std::vector<node_keeper::Actor> up_actors;

  for (auto address : request->addresses()) {
    std::cout << "address: " << address << std::endl;
    up_actors.push_back({address});
  }

  // Todo(Yujia.Li): 把新起来的actor加入到self_里。
  membership::UpdateMessage message;
  message.InitAsActorsUpMessage(cluster_membership_.GetSelf(),
                                cluster_membership_.IncreaseIncarnation(),
                                up_actors);
  auto serialized = message.SerializeToString();
  gossip::Payload payload(serialized.data(), serialized.size());
  cluster_membership_.SendGossip(payload);

  return ::grpc::Status::OK;
}

::grpc::Status GRPCImpl::ActorSystemUp(::grpc::ServerContext* context,
                                       const ::google::protobuf::Empty* request,
                                       ::google::protobuf::Empty* response) {
  membership::UpdateMessage message;
  message.InitAsActorSystemUpMessage(cluster_membership_.GetSelf(),
                                cluster_membership_.IncreaseIncarnation());
  auto serialized = message.SerializeToString();
  gossip::Payload payload(serialized.data(), serialized.size());
  cluster_membership_.SendGossip(payload);

  return ::grpc::Status::OK;
}

::grpc::Status GRPCImpl::Subscribe(::grpc::ServerContext* context,
                                   const ::SubscribeRequest* request,
                                   ::grpc::ServerWriter<::Event>* writer) {
  auto channel = AddChannel();
  for (auto item = channel->Get(); item.first; item = channel->Get()) {
    ::MemberEvent member_event;
    UpdateMember(member_event.mutable_member(), item.second.member);
    if (item.second.type == MemberEvent::kMemberUp) {
      member_event.set_status(::MemberEvent::UP);
    } else if (item.second.type == MemberEvent::kMemberDown) {
      member_event.set_status(::MemberEvent::DOWN);
    } else if (item.second.type == MemberEvent::kActorsUp) {
      member_event.set_status(::MemberEvent::ACTORS_UP);
      auto actors = member_event.mutable_actors();
      for (auto& actor : item.second.actors) {
        actors->add_addresses(actor.address);
      }
    } else if (item.second.type == MemberEvent::kActorSystemDown) {
      member_event.set_status(::MemberEvent::ACTOR_SYSTEM_DOWN);
    } else if (item.second.type == MemberEvent::kActorSystemUp) {
      member_event.set_status(::MemberEvent::ACTOR_SYSTEM_UP);
    }
    ::Event event;
    event.set_type(Event_Type_MEMBER_CHANGED);
    event.mutable_data()->PackFrom(member_event);
    writer->Write(event);
  }
  RemoveChannel(channel);
  return ::grpc::Status::OK;
}

void GRPCImpl::Notify(const std::vector<MemberEvent>& events) {
  for (auto& event : events) {
    switch (event.type) {
      case MemberEvent::kMemberUp:
        members_.insert(event.member);
        break;
      case MemberEvent::kMemberDown:
        members_.erase(event.member);
        break;
      case MemberEvent::kActorsUp:
        // Todo:(存储actors)
        break;
      case MemberEvent::kActorSystemDown:
        // Todo:
        break;
      case MemberEvent::kActorSystemUp:
        // Todo:
        break;
    }
  }

  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& channel : channels_) {
    for (auto& event : events) {
      channel.Put(event);
    }
  }
}
GRPCImpl::GRPCImpl(membership::Membership& cluster_membership)
    : cluster_membership_(cluster_membership) {}

}  // namespace node_keeper
