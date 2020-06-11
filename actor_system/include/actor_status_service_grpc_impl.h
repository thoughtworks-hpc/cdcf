//
// Created by Mingfei Deng on 2020/6/9.
//

#ifndef CDCF_ACTOR_STATUS_SERVICE_GRPC_IMPL_H
#define CDCF_ACTOR_STATUS_SERVICE_GRPC_IMPL_H

#include <grpcpp/server.h>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../include/actor_status_monitor.h"
#include "../src/node_monitor.grpc.pb.h"

class ActorStatusServiceGprcImpl final : public ::NodeActorMonitor::Service {
 public:
  ActorStatusServiceGprcImpl(caf::actor_system& actorSystem,
                             ActorStatusMonitor& actorStatusMonitor,
                             uint16_t server_port = 50001)
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

#endif  // CDCF_ACTOR_STATUS_SERVICE_GRPC_IMPL_H
