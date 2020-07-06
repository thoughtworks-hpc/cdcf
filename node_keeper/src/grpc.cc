/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

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

::grpc::Status GRPCImpl::Subscribe(::grpc::ServerContext* context,
                                   const ::SubscribeRequest* request,
                                   ::grpc::ServerWriter<::Event>* writer) {
  auto channel = AddChannel();
  for (auto item = channel->Get(); item.first; item = channel->Get()) {
    ::MemberEvent member_event;
    UpdateMember(member_event.mutable_member(), item.second.member);
    if (item.second.type == MemberEvent::kMemberUp) {
      member_event.set_status(::MemberEvent::UP);
    } else {
      member_event.set_status(::MemberEvent::DOWN);
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
    }
  }

  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& channel : channels_) {
    for (auto& event : events) {
      channel.Put(event);
    }
  }
}
}  // namespace node_keeper
