/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include <grpcpp/create_channel.h>

#include "./node_monitor.grpc.pb.h"
#include "cdcf/cdcf_config.h"

class ClientConfig : public cdcf::CDCFConfig {
 public:
  std::string server_host = "localhost:50051";
  std::string actor_node_host = "";
  uint16_t port = 50052;

  ClientConfig() {
    opt_group{custom_options_, "global"}
        .add(server_host, "server_host,S",
             "server ip and port, example: 10.13.2.2:50051")
        .add(actor_node_host, "node,N", "get one node actor system status")
        .add(port, "port,P",
             "actor system status service port, default is 50052");
  }
};

void GetActorSystemStatus(const std::string& host, uint16_t port) {
  auto actor_system_channel = grpc::CreateChannel(
      host + ":" + std::to_string(port), grpc::InsecureChannelCredentials());
  ::NodeActorMonitor::Stub actor_system_client(actor_system_channel);

  grpc::ClientContext query_context;
  ::ActorStatus actor_status;
  grpc::Status status =
      actor_system_client.GetNodeActorStatus(&query_context, {}, &actor_status);

  if (!status.ok()) {
    std::cout << "get actor system status from (" << host << ":" << port
              << ") status failed, error message: " << status.error_message()
              << std::endl;
    return;
  }

  std::cout << "Get actor system status from: " << host << ":" << port
            << std::endl;

  std::cout << std::endl;
  std::cout << "Actor executor: " << actor_status.actor_worker() << std::endl;
  std::cout << "Total actor: " << actor_status.actor_infos_size() << std::endl;
  std::cout << std::endl;

  for (auto& one_actor_info : actor_status.actor_infos()) {
    std::cout << " actor: id=(" << one_actor_info.id() << ")" << std::endl;
    std::cout << "  name: " << one_actor_info.name() << std::endl;
    std::cout << "  description: " << one_actor_info.description() << std::endl;
    std::cout << std::endl;
  }
}

int GetNodeStatus(const std::string& host) {
  auto node_status_channel =
      grpc::CreateChannel(host, grpc::InsecureChannelCredentials());
  ::NodeMonitor::Stub client(node_status_channel);

  grpc::ClientContext query_context;
  ::AllNodeStatus all_node_status;
  grpc::Status status =
      client.GetAllNodeStatus(&query_context, {}, &all_node_status);

  if (!status.ok()) {
    std::cout << "Get cluster node ip:port= (" << host
              << ") status failed, error message:" << status.error_message()
              << std::endl;
    return 1;
  }

  std::cout << "Get cluster node status from:" << host << std::endl;
  std::cout << "Total node:" << all_node_status.node_status_size() << std::endl;

  for (auto& one_node_status : all_node_status.node_status()) {
    std::cout << "Cluster node ip: " << one_node_status.ip() << std::endl;

    if ("" == one_node_status.error_message()) {
      std::cout << "  Node name: " << one_node_status.node_name() << std::endl;
      std::cout << "  Node role: " << one_node_status.node_role() << std::endl;
      std::cout << "  Cpu use rate: " << one_node_status.cpu_use_rate() * 100
                << "%" << std::endl;
      std::cout << "  Memory use rate: " << one_node_status.mem_use_rate() * 100
                << "%" << std::endl;
      std::cout << "  Max memory: " << one_node_status.max_memory() << "kB"
                << std::endl;
      std::cout << "  Use memory: " << one_node_status.use_memory() << "kB"
                << std::endl;
    } else {
      std::cout << " error: " << one_node_status.error_message() << std::endl;
    }

    std::cout << std::endl;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  ClientConfig input_parameter;
  cdcf::CDCFConfig::RetValue parse_ret = input_parameter.parse_config(argc, argv);

  if (parse_ret != cdcf::CDCFConfig::RetValue::kSuccess) {
    return 1;
  }

  if ("" != input_parameter.actor_node_host) {
    GetActorSystemStatus(input_parameter.actor_node_host, input_parameter.port);
    return 0;
  } else {
    return GetNodeStatus(input_parameter.server_host);
  }
}
