/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

namespace {
::Member& UpdateMember(::Member* to, const membership::Member& from) {
  to->set_name(from.GetNodeName());
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
  auto& channel = AddChannel();
  for (auto item = channel.Get(); item.first;) {
    ::Member member;
    UpdateMember(&member, item.second.member);
    ::Event event;
    event.set_type(Event_Type_MEMBER_CHANGED);
    event.mutable_data()->PackFrom(member);
    writer->Write(event);
  }
  return ::grpc::Status::OK;
}
}  // namespace node_keeper
