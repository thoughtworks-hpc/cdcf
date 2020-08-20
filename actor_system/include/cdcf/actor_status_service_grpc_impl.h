/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_STATUS_SERVICE_GRPC_IMPL_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_STATUS_SERVICE_GRPC_IMPL_H_

#include <grpcpp/server.h>

#include <memory>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../src/node_monitor.grpc.pb.h"
#include "actor_status_monitor.h"

namespace cdcf {
class ActorStatusServiceGrpcImpl final : public ::NodeActorMonitor::Service {
 public:
  ActorStatusServiceGrpcImpl(caf::actor_system& actorSystem,
                             ActorStatusMonitor& actorStatusMonitor,
                             uint16_t server_port = 50052)
      : actor_system_(actorSystem),
        actor_status_monitor_(actorStatusMonitor),
        server_port_(server_port) {}

  ::grpc::Status GetNodeActorStatus(::grpc::ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    ::ActorStatus* response) override;
  void Run();
  void RunWithWait();

 private:
  caf::actor_system& actor_system_;
  ActorStatusMonitor& actor_status_monitor_;
  uint16_t server_port_;
  std::unique_ptr<grpc::Server> server_;
};

}  // namespace cdcf

#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_STATUS_SERVICE_GRPC_IMPL_H_
