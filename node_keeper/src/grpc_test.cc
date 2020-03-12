/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/grpc.h"

#include <gmock/gmock.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <chrono>
#include <future>
#include <memory>
#include <thread>

using node_keeper::GRPCImpl;
using node_keeper::GRPCServer;
using testing::Eq;

class GRPCTest : public ::testing::Test {
 protected:
  GRPCTest() {}

  void SetUp() override {
    std::vector<grpc::Service*> services = {&service_};
    server_ = std::make_unique<GRPCServer>(server_address_, services);
    ResetStub();
  }

  void TearDown() override {
    service_.Close();
    server_.reset();
  }

  void ResetStub() {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        server_address_, grpc::InsecureChannelCredentials());
    stub_ = NodeKeeper::NewStub(channel);
  }

  std::unique_ptr<NodeKeeper::Stub> stub_;
  /* FIXME: It would be better to find a unused port dynamically like
   * `grpc_pick_unused_port_or_die`. */
  const std::string server_address_{"localhost:30002"};
  GRPCImpl service_;
  std::unique_ptr<GRPCServer> server_;
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

TEST_F(GRPCTest, ShouldReturnBySubscribeAfterDifferentNodeUpAndDown) {
  bool server_done = false, client_done = false;
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientReader<::Event>> reader(
      stub_->Subscribe(&context, {}));
  auto future = std::async([&]() {
    std::vector<::Event> events;
    client_done = true;
    for (::Event event; !server_done && reader->Read(&event);) {
      events.push_back(event);
    }
    return events;
  });
  while (!client_done) {
  }

  /* FIXME: sleep 1s to wait for client invoke `Read`, should find a better
   * alternative to solve uncertain sequence issue */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  service_.Notify({{node_keeper::MemberEvent::kMemberUp, node_a_}});
  server_done = true;
  auto events = future.get();

  EXPECT_THAT(events.size(), Eq(1));
  EXPECT_THAT(events[0].type(), Eq(Event_Type_MEMBER_CHANGED));
  ::Member member;
  events[0].data().UnpackTo(&member);
  EXPECT_THAT(member.name(), Eq(node_a_.GetNodeName()));
  EXPECT_THAT(member.host(), Eq(node_a_.GetIpAddress()));
  EXPECT_THAT(member.port(), Eq(node_a_.GetPort()));
}
