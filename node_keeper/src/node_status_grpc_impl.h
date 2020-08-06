/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
#define NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
#include "src/membership.h"
#include "src/node_monitor.grpc.pb.h"
#include "src/node_run_status_factory.h"

namespace node_keeper {
class NodeStatusGRPCImpl final : public ::NodeMonitor::Service {
 public:
  explicit NodeStatusGRPCImpl(membership::Membership& clusterMember);
  explicit NodeStatusGRPCImpl(membership::Membership& clusterMember,
                              NodeRunStatusFactory& nodeRunStatusFactory);
  ~NodeStatusGRPCImpl() override;
  ::grpc::Status GetStatus(::grpc::ServerContext* context,
                           const ::google::protobuf::Empty* request,
                           ::NodeStatus* response) override;
  ::grpc::Status GetAllNodeStatus(::grpc::ServerContext* context,
                                  const ::google::protobuf::Empty* request,
                                  ::AllNodeStatus* response) override;

 private:
  membership::Membership& cluster_member_;
  //TODO:: factory_class
  NodeRunStatusFactory& node_run_statues_factory_;
};
}  // namespace node_keeper

#endif  // NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
