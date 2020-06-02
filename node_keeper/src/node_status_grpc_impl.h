/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
#define NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
#include "src/membership.h"
#include "src/node_monitor.grpc.pb.h"

namespace node_keeper {
class NodeStatusGRPCImpl final : public ::NodeMonitor::Service {
 public:
  explicit NodeStatusGRPCImpl(membership::Membership& clusterMember);
  ~NodeStatusGRPCImpl() override;
  ::grpc::Status GetStatus(::grpc::ServerContext* context,
                           const ::google::protobuf::Empty* request,
                           ::NodeStatus* response) override;
  ::grpc::Status GetAllNodeStatus(::grpc::ServerContext* context,
                                  const ::google::protobuf::Empty* request,
                                  ::AllNodeStatus* response) override;

 private:
  membership::Membership& cluster_member_;
};
}  // namespace node_keeper

#endif  // NODE_KEEPER_SRC_NODE_STATUS_GRPC_IMPL_H_
