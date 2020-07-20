/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/node_status_grpc_impl.h"

#include <grpcpp/create_channel.h>

#include <string>
#include <vector>

#include "./node_run_status.h"

node_keeper::NodeStatusGRPCImpl::~NodeStatusGRPCImpl() = default;

node_keeper::NodeStatusGRPCImpl::NodeStatusGRPCImpl(
    membership::Membership& clusterMember)
    : cluster_member_(clusterMember) {}

grpc::Status node_keeper::NodeStatusGRPCImpl::GetStatus(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::NodeStatus* response) {
  NodeRunStatus* node_run_status = NodeRunStatus::GetInstance();
  if (nullptr == node_run_status) {
    std::cout << "Can't get node status, os not match." << std::endl;
    return ::grpc::Status(grpc::UNAVAILABLE,
                          "Can't get node status, os not match.");
  }

  MemoryStatus memory_status{};
  int ret = node_run_status->GetMemoryState(memory_status);
  if (0 != ret) {
    std::cout << "Can't get memory status, please connect the administrator."
              << std::endl;
    return ::grpc::Status(
        grpc::UNAVAILABLE,
        "Can't get memory status, please connect the administrator.");
  }

  double cpu_rate = node_run_status->GetCpuRate();

  response->set_max_memory(memory_status.max_memory);
  response->set_use_memory(memory_status.use_memory);
  response->set_mem_use_rate(memory_status.useRate);
  response->set_cpu_use_rate(cpu_rate);

  return ::grpc::Status::OK;
}

::grpc::Status node_keeper::NodeStatusGRPCImpl::GetAllNodeStatus(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::AllNodeStatus* response) {
  std::vector<membership::Member> members = cluster_member_.GetMembers();

  for (auto& member : members) {
    std::string host_ip = member.GetIpAddress();
    std::string host = host_ip + ":50051";
    auto channel =
        grpc::CreateChannel(host, grpc::InsecureChannelCredentials());
    ::NodeMonitor::Stub client(channel);
    grpc::ClientContext query_context;

    ::NodeStatus resp;
    grpc::Status status = client.GetStatus(&query_context, {}, &resp);

    auto new_node_status = response->add_node_status();

    if (status.ok()) {
      new_node_status->set_node_name(member.GetNodeName());
      new_node_status->set_node_role(member.GetRole());
      new_node_status->set_max_memory(resp.max_memory());
      new_node_status->set_use_memory(resp.use_memory());
      new_node_status->set_mem_use_rate(resp.mem_use_rate());
      new_node_status->set_cpu_use_rate(resp.cpu_use_rate());
      new_node_status->set_ip(host_ip);
    } else {
      new_node_status->set_ip(host_ip);
      new_node_status->set_error_message(status.error_message());
    }
  }

  return ::grpc::Status::OK;
}
