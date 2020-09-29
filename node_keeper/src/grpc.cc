/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

#include "src/membership_message.h"

namespace {
::Member& UpdateMember(::Member* to, const membership::Member& from) {
  to->set_name(from.GetNodeName());
  to->set_hostname(from.GetHostName());
  to->set_role(from.GetRole());
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

::grpc::Status GRPCImpl::ActorSystemUp(::grpc::ServerContext* context,
                                       const ::google::protobuf::Empty* request,
                                       ::google::protobuf::Empty* response) {
  CDCF_LOGGER_INFO("receive actor system up from self actor system.");
  cluster_membership_.SetSelfActorSystemUp();
  cluster_membership_.SendSelfActorSystemUpGossip();
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
      case MemberEvent::kActorSystemDown:
        break;
      case MemberEvent::kActorSystemUp:
        break;
    }
  }

  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& channel : channels_) {
    for (auto& event : events) {
      if (event.member.GetIpAddress() == host_ ||
          event.member.GetHostName() == host_) {
        CDCF_LOGGER_INFO("Find local node event, host: {} ignored", host_);
        continue;
      }
      channel.Put(event);
    }
  }
}

GRPCImpl::GRPCImpl(membership::Membership& cluster_membership)
    : cluster_membership_(cluster_membership) {}

void GRPCImpl::SetHost(const std::string& host) { host_ = host; }

}  // namespace node_keeper
