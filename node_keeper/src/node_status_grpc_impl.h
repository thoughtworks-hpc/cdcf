//
// Created by Mingfei Deng on 2020/5/28.
//

#ifndef CDCF_NODE_STATUS_GRPC_IMPL_H
#define CDCF_NODE_STATUS_GRPC_IMPL_H
#include "src/membership.h"
#include "src/node_monitor.grpc.pb.h"

namespace node_keeper {
class NodeStatusGRPCImpl final : public ::NodeMonitor::Service {
 public:
  NodeStatusGRPCImpl(membership::Membership& clusterMember);
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

#endif  // CDCF_NODE_STATUS_GRPC_IMPL_H
