/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "cdcf/actor_status_service_grpc_impl.h"

#include <cdcf/logger.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

::grpc::Status ActorStatusServiceGrpcImpl::GetNodeActorStatus(
    ::grpc::ServerContext *context, const ::google::protobuf::Empty *request,
    ::ActorStatus *response) {
  std::vector<cdcf::actor_system::ActorStatusMonitor::ActorInfo> actor_list =
      actor_status_monitor_.GetActorStatus();

  for (cdcf::actor_system::ActorStatusMonitor::ActorInfo &actor : actor_list) {
    ActorInfo *actor_info = response->add_actor_infos();
    actor_info->set_id(actor.id);
    actor_info->set_name(actor.name);
    actor_info->set_description(actor.description);
  }

  caf::scheduler::abstract_coordinator &sch = actor_system_.scheduler();
  response->set_actor_worker(
      static_cast<google::protobuf::int32>(sch.num_workers()));

  return ::grpc::Status::OK;
}
void ActorStatusServiceGrpcImpl::Run() {
  std::string server_address("0.0.0.0:" + std::to_string(server_port_));
  CDCF_LOGGER_INFO("actor status service up at port:{}", server_port_);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(this);
  server_ = builder.BuildAndStart();
}

void ActorStatusServiceGrpcImpl::RunWithWait() {
  Run();
  server_->Wait();
}
