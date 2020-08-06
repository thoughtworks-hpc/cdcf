//
// Created by Yuecheng Pei on 2020/8/5.
//

#include "node_status_grpc_impl.h"

#include <gmock/gmock.h>
#include <grpcpp/create_channel.h>

#include "config.h"
#include "node_keeper.h"
#include "node_run_status.h"

using testing::Eq;
class MockNodeRunStatus : public NodeRunStatus {
 public:
  MOCK_METHOD(NodeRunStatus*, GetInstance, ());
  MOCK_METHOD(double, GetCpuRate, ());
  MOCK_METHOD(int, GetMemoryState, (MemoryStatus & memory_info));
};

class MockMembership : public membership::Membership {
  MOCK_METHOD(std::vector<membership::Member>, GetMembers, ());
};

class MockNodeRunStatusFactory : public NodeRunStatusFactory {
  MOCK_METHOD(NodeRunStatus*, GetInstance, ());
};

TEST(GetStatus, happy_path) {
  //  node_keeper::Config config;
  //  config.parse_config({"--name=node0", "--host=127.0.0.1", "--port=5000"});
  //  node_keeper::NodeKeeper keeper(config);
  //  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
  //  keeper.Run();

  MockMembership membership_;
  MockNodeRunStatusFactory mockNodeRunStatusFactory;
  node_keeper::NodeStatusGRPCImpl node_status_service(membership_);

  auto channel =
      grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
  ::NodeMonitor::Stub client(channel);
  grpc::ClientContext query_context;
  ::NodeStatus resp;

  grpc::Status status = client.GetStatus(&query_context, {}, &resp);

}
