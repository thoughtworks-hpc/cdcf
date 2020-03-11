/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

#include <gmock/gmock.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <memory>

using node_keeper::GRPCImpl;
using testing::Eq;

class GRPCTest : public ::testing::Test {
 protected:
  GRPCTest() {}

  void SetUp() override {
    /* FIXME: It would be better to find a unused port dynamically like
     * `grpc_pick_unused_port_or_die`. */
    int port = 30002;
    server_address_ << "localhost:" << port;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_.str(),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();

    ResetStub();
  }

  void TearDown() override { server_->Shutdown(); }

  void ResetStub() {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        server_address_.str(), grpc::InsecureChannelCredentials());
    stub_ = NodeKeeper::NewStub(channel);
  }

  std::unique_ptr<NodeKeeper::Stub> stub_;
  std::unique_ptr<grpc::Server> server_;
  std::ostringstream server_address_;
  GRPCImpl service_;
  membership::Member node_a_ = {"node_a", "localhost", 8834};
  membership::Member node_b_ = {"node_b", "localhost", 8835};
};

TEST_F(GRPCTest, ShouldReturnNoneMemberByGetMembers) {
  ::GetMembersReply reply;
  grpc::ClientContext context;
  auto status = stub_->GetMembers(&context, {}, &reply);

  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(reply.members().empty());
}

TEST_F(GRPCTest, ShouldReturnOneMemberByGetMembersAfterNodeUp) {
  service_.Notify({{node_keeper::MemberEvent::kMemberUp, node_a_}});

  ::GetMembersReply reply;
  grpc::ClientContext context;
  auto status = stub_->GetMembers(&context, {}, &reply);

  EXPECT_TRUE(status.ok());
  EXPECT_THAT(reply.members().size(), Eq(1));
}

TEST_F(GRPCTest, ShouldReturnNoneMemberByGetMembersAfterSameNodeUpAndDown) {
  service_.Notify({{node_keeper::MemberEvent::kMemberUp, node_a_}});
  service_.Notify({{node_keeper::MemberEvent::kMemberDown, node_a_}});

  ::GetMembersReply reply;
  grpc::ClientContext context;
  auto status = stub_->GetMembers(&context, {}, &reply);

  EXPECT_TRUE(status.ok());
  EXPECT_THAT(reply.members().size(), Eq(0));
}

TEST_F(GRPCTest, ShouldReturnOneMemberByGetMembersAfterDifferentNodeUpAndDown) {
  service_.Notify({{node_keeper::MemberEvent::kMemberUp, node_a_}});
  service_.Notify({{node_keeper::MemberEvent::kMemberDown, node_b_}});

  ::GetMembersReply reply;
  grpc::ClientContext context;
  auto status = stub_->GetMembers(&context, {}, &reply);

  EXPECT_TRUE(status.ok());
  EXPECT_THAT(reply.members().size(), Eq(1));
}
