/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "./node_status_grpc_impl.h"

#include <gmock/gmock.h>
#include <grpcpp/create_channel.h>

#include <string>
#include <vector>

#include "./grpc.h"
#include "./node_keeper.h"

using testing::Eq;
class MockNodeRunStatus : public NodeRunStatus {
 public:
  MockNodeRunStatus(FILE* memoryFile, FILE* cpuFile)
      : NodeRunStatus(memoryFile, cpuFile) {}
  MOCK_METHOD(double, GetCpuRate, ());
  MOCK_METHOD(int, GetMemoryState, (MemoryStatus&));
};

class MockMembership : public membership::Membership {
 public:
  MOCK_METHOD(std::vector<membership::Member>, GetMembers, (), (const));
};

class MockNodeRunStatusFactory : public NodeRunStatusFactory {
 public:
  MOCK_METHOD(NodeRunStatus*, GetInstance, ());
};

const double kMockCpuRateNode1 = 57.8;
const double kMockCpuRateNode2 = 68.7;
const int kMockMaxMemory = 100;
const int kMockUseMemoryNode1 = 37;
const int kMockUseRateNode1 = 37;
const int kMockUseMemoryNode2 = 89;
const int kMockUseRateNode2 = 89;

TEST(GetStatus, happy_path_call_get_status_without_grpc) {
  MockMembership membership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  node_keeper::NodeStatusGRPCImpl nodeStatusGRPCImpl(membership_,
                                                     mockNodeRunStatusFactory);
  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(1)
      .WillOnce(testing::Return(&mockNodeRunStatus));
  EXPECT_CALL(mockNodeRunStatus, GetCpuRate())
      .Times(1)
      .WillOnce(testing::Return(kMockCpuRateNode1));

  MemoryStatus memory_status{kMockMaxMemory, kMockUseMemoryNode1,
                             kMockUseRateNode1};

  EXPECT_CALL(mockNodeRunStatus, GetMemoryState(testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgReferee<0>(memory_status),
                               testing::Return(0)));

  grpc::ServerContext query_context;
  ::NodeStatus resp;
  grpc::Status status = nodeStatusGRPCImpl.GetStatus(&query_context, {}, &resp);

  EXPECT_EQ(status.ok(), true);
  EXPECT_EQ(resp.cpu_use_rate(), kMockCpuRateNode1);
  EXPECT_EQ(resp.use_memory(), kMockUseMemoryNode1);
  EXPECT_EQ(resp.max_memory(), kMockMaxMemory);
  EXPECT_EQ(resp.mem_use_rate(), kMockUseRateNode1);
}

TEST(GetStatus, happy_path_use_grpc_call) {
  MockMembership membership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  std::string server_address("127.0.0.1:50051");
  node_keeper::GRPCImpl service(membership_);
  node_keeper::NodeStatusGRPCImpl node_status_service(membership_,
                                                      mockNodeRunStatusFactory);
  node_keeper::GRPCServer server(server_address,
                                 {&service, &node_status_service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(1)
      .WillOnce(testing::Return(&mockNodeRunStatus));

  MemoryStatus memory_status{kMockMaxMemory, kMockUseMemoryNode1,
                             kMockUseRateNode1};
  EXPECT_CALL(mockNodeRunStatus, GetMemoryState(testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgReferee<0>(memory_status),
                               testing::Return(0)));

  EXPECT_CALL(mockNodeRunStatus, GetCpuRate())
      .Times(1)
      .WillOnce(testing::Return(kMockCpuRateNode1));

  auto channel = grpc::CreateChannel("127.0.0.1:50051",
                                     grpc::InsecureChannelCredentials());
  ::NodeMonitor::Stub client(channel);

  grpc::ClientContext query_context;
  ::NodeStatus resp;
  grpc::Status status = client.GetStatus(&query_context, {}, &resp);

  EXPECT_EQ(status.ok(), true);
  EXPECT_EQ(resp.cpu_use_rate(), kMockCpuRateNode1);
  EXPECT_EQ(resp.use_memory(), kMockUseMemoryNode1);
  EXPECT_EQ(resp.max_memory(), kMockMaxMemory);
  EXPECT_EQ(resp.mem_use_rate(), kMockUseRateNode1);
}

TEST(GetStatus, can_not_get_node_status) {
  MockMembership membership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  node_keeper::NodeStatusGRPCImpl nodeStatusGRPCImpl(membership_,
                                                     mockNodeRunStatusFactory);
  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(1)
      .WillOnce(testing::Return(nullptr));

  grpc::ServerContext query_context;
  ::NodeStatus resp;
  grpc::Status status = nodeStatusGRPCImpl.GetStatus(&query_context, {}, &resp);

  EXPECT_EQ(status.ok(), false);
  EXPECT_EQ(status.error_code(), grpc::UNAVAILABLE);
  EXPECT_EQ(status.error_message(), "Can't get node status, os not match.");
}

TEST(GetStatus, can_not_get_memory_state) {
  MockMembership membership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  node_keeper::NodeStatusGRPCImpl nodeStatusGRPCImpl(membership_,
                                                     mockNodeRunStatusFactory);
  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(1)
      .WillOnce(testing::Return(&mockNodeRunStatus));

  EXPECT_CALL(mockNodeRunStatus, GetMemoryState(testing::_))
      .Times(1)
      .WillOnce(testing::Return(-1));
  grpc::ServerContext query_context;
  ::NodeStatus resp;
  grpc::Status status = nodeStatusGRPCImpl.GetStatus(&query_context, {}, &resp);

  EXPECT_EQ(status.ok(), false);
  EXPECT_EQ(status.error_code(), grpc::UNAVAILABLE);
  EXPECT_EQ(status.error_message(),
            "Can't get memory status, please connect the administrator.");
}

TEST(GetAllNodeStatus, happy_path_use_grpc_call) {
  MockMembership mockMembership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  std::string server_address("127.0.0.1:50051");
  node_keeper::GRPCImpl service(mockMembership_);
  node_keeper::NodeStatusGRPCImpl node_status_service(mockMembership_,
                                                      mockNodeRunStatusFactory);
  node_keeper::GRPCServer server(server_address,
                                 {&service, &node_status_service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(2)
      .WillOnce(testing::Return(&mockNodeRunStatus))
      .WillOnce(testing::Return(&mockNodeRunStatus));

  std::vector<membership::Member> members;
  members.push_back(*new membership::Member(
      "node1", "127.0.0.1", 50051, "host_name_node_1", "uid1", "worker"));
  members.push_back(*new membership::Member(
      "node2", "127.0.0.1", 50051, "host_name_node_2", "uid2", "worker"));
  EXPECT_CALL(mockMembership_, GetMembers())
      .Times(1)
      .WillOnce(testing::Return(members));

  MemoryStatus memory_status_1{kMockMaxMemory, kMockUseMemoryNode1,
                               kMockUseRateNode1};
  MemoryStatus memory_status_2{kMockMaxMemory, kMockUseMemoryNode2,
                               kMockUseRateNode2};

  EXPECT_CALL(mockNodeRunStatus, GetMemoryState(testing::_))
      .Times(2)
      .WillOnce(testing::DoAll(testing::SetArgReferee<0>(memory_status_1),
                               testing::Return(0)))
      .WillOnce(testing::DoAll(testing::SetArgReferee<0>(memory_status_2),
                               testing::Return(0)));

  EXPECT_CALL(mockNodeRunStatus, GetCpuRate())
      .Times(2)
      .WillOnce(testing::Return(kMockCpuRateNode1))
      .WillOnce(testing::Return(kMockCpuRateNode2));

  grpc::ServerContext query_context;
  auto* resp = new AllNodeStatus();
  grpc::Status status =
      node_status_service.GetAllNodeStatus(&query_context, {}, resp);
  EXPECT_EQ(status.ok(), true);
  EXPECT_EQ(resp->node_status_size(), 2);

  EXPECT_EQ(resp->node_status(0).cpu_use_rate(), kMockCpuRateNode1);
  EXPECT_EQ(resp->node_status(0).use_memory(), kMockUseMemoryNode1);
  EXPECT_EQ(resp->node_status(0).max_memory(), kMockMaxMemory);
  EXPECT_EQ(resp->node_status(0).mem_use_rate(), kMockUseRateNode1);

  EXPECT_EQ(resp->node_status(1).cpu_use_rate(), kMockCpuRateNode2);
  EXPECT_EQ(resp->node_status(1).use_memory(), kMockUseMemoryNode2);
  EXPECT_EQ(resp->node_status(1).max_memory(), kMockMaxMemory);
  EXPECT_EQ(resp->node_status(1).mem_use_rate(), kMockUseRateNode2);
}

TEST(GetAllNodeStatus, one_node_is_unavailable) {
  MockMembership mockMembership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  MockNodeRunStatus mockNodeRunStatus(nullptr, nullptr);
  std::string server_address("127.0.0.1:50051");
  node_keeper::GRPCImpl service(mockMembership_);
  node_keeper::NodeStatusGRPCImpl node_status_service(mockMembership_,
                                                      mockNodeRunStatusFactory);
  node_keeper::GRPCServer server(server_address,
                                 {&service, &node_status_service});
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  EXPECT_CALL(mockNodeRunStatusFactory, GetInstance())
      .Times(1)
      .WillOnce(testing::Return(&mockNodeRunStatus));

  std::vector<membership::Member> members;
  members.push_back(*new membership::Member(
      "node1", "127.0.0.1", 50051, "host_name_node_1", "uid1", "worker"));
  members.push_back(*new membership::Member("node2", "Unavailable Address",
                                            50051, "host_name_node_2", "uid2",
                                            "worker"));
  EXPECT_CALL(mockMembership_, GetMembers())
      .Times(1)
      .WillOnce(testing::Return(members));

  MemoryStatus memory_status_1{100, 37, 37};

  EXPECT_CALL(mockNodeRunStatus, GetMemoryState(testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgReferee<0>(memory_status_1),
                               testing::Return(0)));

  EXPECT_CALL(mockNodeRunStatus, GetCpuRate())
      .Times(1)
      .WillOnce(testing::Return(57.8));

  grpc::ServerContext query_context;
  auto* resp = new AllNodeStatus();
  grpc::Status status =
      node_status_service.GetAllNodeStatus(&query_context, {}, resp);
  EXPECT_EQ(status.ok(), true);
  EXPECT_EQ(resp->node_status_size(), 2);

  EXPECT_EQ(resp->node_status(0).cpu_use_rate(), kMockCpuRateNode1);
  EXPECT_EQ(resp->node_status(0).use_memory(), kMockUseMemoryNode1);
  EXPECT_EQ(resp->node_status(0).max_memory(), kMockMaxMemory);
  EXPECT_EQ(resp->node_status(0).mem_use_rate(), kMockUseRateNode1);

  EXPECT_EQ(resp->node_status(1).cpu_use_rate(), 0);
  EXPECT_EQ(resp->node_status(1).use_memory(), 0);
  EXPECT_EQ(resp->node_status(1).max_memory(), 0);
  EXPECT_EQ(resp->node_status(1).mem_use_rate(), 0);
  EXPECT_EQ(resp->node_status(1).ip(), "Unavailable Address");
  EXPECT_EQ(resp->node_status(1).error_message(), "DNS resolution failed");
}
