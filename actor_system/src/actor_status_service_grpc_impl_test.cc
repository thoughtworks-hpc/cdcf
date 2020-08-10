/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../include/actor_status_service_grpc_impl.h"

#include <gmock/gmock.h>
#include <grpcpp/create_channel.h>

caf::behavior Subtraction(caf::event_based_actor*) {
  return {[=](int a, int b) { return a - b; }};
}

class actor_status_service_test : public testing::Test {
 protected:
  void SetUp() override {
    test_actor1_ = actor_system_.spawn(Subtraction);
    test_actor2_ = actor_system_.spawn(Subtraction);
  }

  void TearDown() override {
    caf::anon_send_exit(test_actor1_, caf::exit_reason::kill);
  }

  caf::actor_system_config config_;
  caf::actor_system actor_system_{config_};
  caf::actor test_actor1_;
  caf::actor test_actor2_;
  ActorStatusMonitor status_monitor_{actor_system_};
};

const std::string kLocalHostAddress = "127.0.0.1:50052";
const std::string kTestActorName1 = "TestActor1";
const std::string kTestActorDescription1 = "test1 description";
const std::string kTestActorName2 = "TestActor2";
const std::string kTestActorDescription2 = "test2 description";

TEST_F(actor_status_service_test, actor_system_has_no_actor) {
  ActorStatusServiceGrpcImpl service_grpc(actor_system_, status_monitor_);
  service_grpc.Run();

  auto actor_system_channel = grpc::CreateChannel(
      kLocalHostAddress, grpc::InsecureChannelCredentials());
  ::NodeActorMonitor::Stub actor_system_client(actor_system_channel);

  grpc::ClientContext query_context;
  ::ActorStatus actor_status;
  grpc::Status status =
      actor_system_client.GetNodeActorStatus(&query_context, {}, &actor_status);

  EXPECT_EQ(0, actor_status.actor_infos().size());
}

TEST_F(actor_status_service_test, actor_system_has_one_actor) {
  ActorStatusServiceGrpcImpl service_grpc(actor_system_, status_monitor_);
  service_grpc.Run();
  status_monitor_.RegisterActor(test_actor1_, kTestActorName1,
                                kTestActorDescription1);

  auto actor_system_channel = grpc::CreateChannel(
      kLocalHostAddress, grpc::InsecureChannelCredentials());
  ::NodeActorMonitor::Stub actor_system_client(actor_system_channel);

  grpc::ClientContext query_context;
  ::ActorStatus actor_status;
  grpc::Status status =
      actor_system_client.GetNodeActorStatus(&query_context, {}, &actor_status);

  EXPECT_EQ(1, actor_status.actor_infos().size());
  auto actor_info = actor_status.actor_infos()[0];
  EXPECT_EQ(kTestActorName1, actor_info.name());
  EXPECT_EQ(test_actor1_.id(), actor_info.id());
  EXPECT_EQ(kTestActorDescription1, actor_info.description());
}

TEST_F(actor_status_service_test, actor_system_has_two_actor) {
  ActorStatusServiceGrpcImpl service_grpc(actor_system_, status_monitor_);
  service_grpc.Run();
  status_monitor_.RegisterActor(test_actor1_, kTestActorName1,
                                kTestActorDescription1);

  status_monitor_.RegisterActor(test_actor2_, kTestActorName2,
                                kTestActorDescription2);

  auto actor_system_channel = grpc::CreateChannel(
      kLocalHostAddress, grpc::InsecureChannelCredentials());
  ::NodeActorMonitor::Stub actor_system_client(actor_system_channel);

  grpc::ClientContext query_context;
  ::ActorStatus actor_status;
  grpc::Status status =
      actor_system_client.GetNodeActorStatus(&query_context, {}, &actor_status);

  EXPECT_EQ(2, actor_status.actor_infos().size());
  auto actor_info2 = actor_status.actor_infos()[0];
  auto actor_info1 = actor_status.actor_infos()[1];
  EXPECT_EQ(kTestActorName1, actor_info1.name());
  EXPECT_EQ(test_actor1_.id(), actor_info1.id());
  EXPECT_EQ(kTestActorDescription1, actor_info1.description());
  EXPECT_EQ(kTestActorName2, actor_info2.name());
  EXPECT_EQ(test_actor2_.id(), actor_info2.id());
  EXPECT_EQ(kTestActorDescription2, actor_info2.description());
}