/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */

#include "src/membership.h"

#include <gmock/gmock.h>

#include <cmath>
#include <vector>

#include "gtest/gtest.h"
#include "src/membership_message.h"
#include "src/mock_gossip.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::Return;

TEST(Member, ShouldOverwriteCompareOperatorRight) {
  membership::Member a("1", "node_a", 4445);
  membership::Member b("1", "node_b", 4445);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_NE(a, b);
  EXPECT_EQ(a, a);
}

// Member
bool CompareMembers(const std::vector<membership::Member>& lhs,
                    const std::vector<membership::Member>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (int i = 0; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

TEST(Membership, CreateOneMember) {
  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(member.GetNodeName(), "node1");
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
  EXPECT_EQ(member.GetPort(), 27777);
}

TEST(Membership, MemberComparisonEqual) {
  membership::Member member1("node1", "127.0.0.1", 27777);
  membership::Member member2("node1", "127.0.0.1", 27777);

  EXPECT_EQ(member1, member2);
}

TEST(Membership, MemberComparisonNotEqual) {
  membership::Member member1("node1", "127.0.0.1", 27777);
  membership::Member member2("node1", "127.0.0.1", 27788);

  EXPECT_NE(member1, member2);
}

// Config
TEST(Membership, ConfigWithHost) {
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);

  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(config.GetHostMember(), member);
}

TEST(Membership, ConfigWithSeedMember) {
  membership::Config config;
  config.AddOneSeedMember("node1", "127.0.0.1", 27777);
  std::vector<membership::Member> seed_members = config.GetSeedMembers();

  std::vector<membership::Member> seed_members_compare;
  seed_members_compare.emplace_back("node1", "127.0.0.1", 27777);

  EXPECT_TRUE(CompareMembers(seed_members, seed_members_compare));
}

TEST(Membership, ConfigWithDuplicateSeedMember) {
  membership::Config config;
  EXPECT_EQ(config.AddOneSeedMember("node1", "127.0.0.1", 27777),
            membership::ErrorCode::MEMBERSHIP_SUCCESS);
  EXPECT_EQ(config.AddOneSeedMember("node1", "127.0.0.1", 27777),
            membership::ErrorCode::MEMBERSHIP_FAILURE);

  std::vector<membership::Member> seed_members = config.GetSeedMembers();
  std::vector<membership::Member> seed_members_compare;
  seed_members_compare.emplace_back("node1", "127.0.0.1", 27777);

  EXPECT_TRUE(CompareMembers(seed_members, seed_members_compare));
}

TEST(Membership, ShouldGetIpWithHostNameWhenSetHostMemberWithHostName) {
  membership::Config config;
  config.SetHostMember("node1", "localhost", 27777);
  auto member = config.GetHostMember();
  EXPECT_EQ(member.GetHostName(), "localhost");
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
}

TEST(Membership, ShouldFailWhenSetHostMemberWithInvaidHostName) {
  membership::Config config;
  EXPECT_EQ(config.SetHostMember("node1", "invalidhostname", 27777),
            membership::MEMBERSHIP_CONFIG_IP_ADDRESS_INVALID);
}

// Message
TEST(Message, CreateUpMessage) {
  membership::UpdateMessage message1;
  membership::Member member{"node2", "127.0.0.1", 28888};
  message1.InitAsUpMessage(member, 1);
  std::string serialized_msg = message1.SerializeToString();

  membership::UpdateMessage message2;
  message2.DeserializeFromString(serialized_msg);
  EXPECT_TRUE(message2.IsUpMessage());
  EXPECT_EQ(message2.GetMember(), member);
  //   EXPECT_EQ(message2.GetIncarnation(), 1);
}

TEST(Message, MessageParseFromArray) {
  membership::UpdateMessage message1;
  membership::Member member{"node2", "127.0.0.1", 28888};
  message1.InitAsUpMessage(member, 1);
  std::string serialized_msg = message1.SerializeToString();

  membership::UpdateMessage message2;
  message2.DeserializeFromArray(serialized_msg.data(), serialized_msg.size());
  EXPECT_TRUE(message2.IsUpMessage());
  EXPECT_EQ(message2.GetMember(), member);
  // EXPECT_EQ(message2.GetIncarnation(), 1);
}

TEST(Message, CreateFullStateMessage) {
  membership::FullStateMessage message_original;
  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};
  message_original.InitAsFullStateMessage(members);

  std::string serialized_msg = message_original.SerializeToString();

  membership::FullStateMessage message_received;
  message_received.DeserializeFromString(serialized_msg);
  auto members_received = message_received.GetMembers();
  EXPECT_TRUE(CompareMembers(members, members_received));
}

// Membership
TEST(Membership, CreateHostMember) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);

  my_membership.Init(std::make_shared<MockTransport>(), config);

  std::vector<membership::Member> members = my_membership.GetMembers();
  EXPECT_EQ(members.size(), 1);
  EXPECT_EQ(members[0], member);
}

TEST(Membership, CreateHostMemberWithEmptyConfig) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;

  EXPECT_EQ(my_membership.Init(std::make_shared<MockTransport>(), config),
            membership::MEMBERSHIP_INIT_HOSTMEMBER_EMPTY);
}

void SimulateReceiveMessage(membership::UpdateMessage message,
                            std::shared_ptr<MockTransport> transport) {
  auto member = message.GetMember();
  gossip::Address address{member.GetIpAddress(), member.GetPort()};
  std::string serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  transport->CallGossipHandler(address, payload);
}

void SimulateReceivingUpMessage(const membership::Member& member,
                                std::shared_ptr<MockTransport> transport) {
  membership::UpdateMessage message;
  message.InitAsUpMessage(member, 1);
  SimulateReceiveMessage(message, transport);
}

void SimulateReceivingDownMessage(const membership::Member& member,
                                  std::shared_ptr<MockTransport> transport) {
  membership::UpdateMessage message;
  message.InitAsDownMessage(member, 1);
  SimulateReceiveMessage(message, transport);
}

void SimulateReceivingSuspectMessage(const membership::Member& member,
                                     std::shared_ptr<MockTransport> transport) {
  membership::UpdateMessage message;
  message.InitAsSuspectMessage(member, 1);
  SimulateReceiveMessage(message, transport);
}

void SimulateReceivingRecoveryMessage(
    const membership::Member& member,
    std::shared_ptr<MockTransport> transport) {
  membership::UpdateMessage message;
  message.InitAsRecoveryMessage(member, 1);
  SimulateReceiveMessage(message, transport);
}

void SimulateReceivingPingMessage(
    const std::map<membership::Member, int>& members,
    std::shared_ptr<MockTransport> transport) {
  membership::PullRequestMessage message;
  message.InitAsPingType(members, {});

  gossip::Address address;
  std::string serialized_msg = message.SerializeToString();

  transport->CallPullHandler(address, serialized_msg.data(),
                             serialized_msg.size());
}

TEST(Membership, NewUpMessageReceived) {
  membership::Membership my_membership;
  auto transport = std::make_shared<MockTransport>();
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  my_membership.Init(transport, config);

  membership::Member member{"node2", "127.0.0.1", 28888};
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));
}

TEST(Membership, NewRecoverMessageReceived) {
  membership::Membership my_membership;
  auto transport = std::make_shared<MockTransport>();
  membership::Config config;
  membership::Member node1{"node1", "127.0.0.1", 27777};
  config.SetHostMember(node1.GetNodeName(), node1.GetIpAddress(),
                       node1.GetPort());
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  my_membership.Init(transport, config);

  membership::Member node2{"node2", "127.0.0.1", 28888};
  SimulateReceivingUpMessage(node2, transport);

  std::vector<membership::Member> members{node1, node2};

  SimulateReceivingSuspectMessage(node2, transport);

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  EXPECT_EQ(1, my_membership.GetSuspects().size());
  EXPECT_EQ(1, my_membership.GetMembers().size());

  SimulateReceivingRecoveryMessage(node2, transport);
  EXPECT_EQ(0, my_membership.GetSuspects().size());
  EXPECT_EQ(2, my_membership.GetMembers().size());
}

TEST(Membership, DuplicateUpMessageReceived) {
  membership::Membership my_membership;
  auto transport = std::make_shared<MockTransport>();
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  my_membership.Init(transport, config);

  membership::Member member{"node2", "127.0.0.1", 28888};
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  SimulateReceivingUpMessage(member, transport);
  SimulateReceivingUpMessage(member, transport);
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));
}

TEST(Membership, NewDownMessageReceived) {
  membership::Membership my_membership;
  auto transport = std::make_shared<MockTransport>();
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  my_membership.Init(transport, config);

  membership::Member member{"node2", "127.0.0.1", 28888};
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));

  membership::Member memberDowned{"node2", "127.0.0.1", 27777};
  SimulateReceivingDownMessage(memberDowned, transport);
  SimulateReceivingDownMessage(memberDowned, transport);

  std::vector<membership::Member> membersAfterDown{
      {"node1", "127.0.0.1", 27777}, {"node1", "127.0.0.1", 28888}};
  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), membersAfterDown));
}

// TEST(Membership, UpMessageRetransmitWithThreeMember) {
//  // start a member
//  membership::Membership node;
//  membership::Config config;
//  auto transport = std::make_shared<MockTransport>();
//  config.SetHostMember("node1", "127.0.0.1", 27777);
//  config.SetRetransmitMultiplier(3);
//
//  int retransmit_limit_one_member =
//      config.GetRetransmitMultiplier() * ceil(log10(2));
//  int retransmit_limit_two_member =
//      config.GetRetransmitMultiplier() * ceil(log10(3));
//  int retransmit_limit_three_member =
//      config.GetRetransmitMultiplier() * ceil(log10(4));
//
//  std::cout << "retransmit_limit_one_member: " << retransmit_limit_one_member
//            << std::endl;
//  std::cout << "retransmit_limit_two_member: " << retransmit_limit_two_member
//            << std::endl;
//  std::cout << "retransmit_limit_three_member: "
//            << retransmit_limit_three_member << std::endl;
//
//  int total_retransmit_limit = retransmit_limit_one_member +
//                               retransmit_limit_two_member +
//                               retransmit_limit_three_member * 2;
//  EXPECT_CALL(*transport, Gossip).Times(total_retransmit_limit);
//
//  node.Init(transport, config);
//
//  std::this_thread::sleep_for(std::chrono::seconds(total_retransmit_limit));
//
//  SimulateReceivingUpMessage({"node2", "127.0.0.1", 28888}, transport);
//  EXPECT_TRUE(CompareMembers(
//      node.GetMembers(),
//      {{"node1", "127.0.0.1", 27777}, {"node2", "127.0.0.1", 28888}}));
//
//  SimulateReceivingUpMessage({"node3", "127.0.0.1", 29999}, transport);
//  EXPECT_TRUE(
//      CompareMembers(node.GetMembers(), {{"node1", "127.0.0.1", 27777},
//                                         {"node2", "127.0.0.1", 28888},
//                                         {"node3", "127.0.0.1", 29999}}));
//
//  SimulateReceivingUpMessage({"node4", "127.0.0.1", 26666}, transport);
//  EXPECT_TRUE(
//      CompareMembers(node.GetMembers(), {{"node4", "127.0.0.1", 26666},
//                                         {"node1", "127.0.0.1", 27777},
//                                         {"node2", "127.0.0.1", 28888},
//                                         {"node3", "127.0.0.1", 29999}}));
//}

TEST(Membership, JoinWithSingleNodeCluster) {
  // start a member
  membership::Membership node;
  membership::Config config;
  auto transport = std::make_shared<MockTransport>();
  config.SetHostMember("node_a", "127.0.0.1", 27777);
  config.AddOneSeedMember("node_b", "127.0.0.1", 28888);

  membership::UpdateMessage message;
  message.InitAsUpMessage({"node_a", "127.0.0.1", 27777}, 1);
  std::string serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  membership::FullStateMessage fullstate_message;
  fullstate_message.InitAsFullStateMessage({{"node_b", "127.0.0.1", 28888},
                                            {"node_c", "127.0.0.1", 29999},
                                            {"node_d", "127.0.0.1", 30000}});
  auto fullstate_message_serialized = fullstate_message.SerializeToString();

  // should send an up message to seed member
  EXPECT_CALL(*transport, Pull(gossip::Address{"127.0.0.1", 28888}, _, _, _))
      .Times(AtLeast(1))
      .WillOnce(Return(std::make_pair(
          gossip::ErrorCode::kOK,
          std::vector<uint8_t>(fullstate_message_serialized.begin(),
                               fullstate_message_serialized.end()))));
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  //  transport->CallPushHandler(gossip::Address{"127.0.0.1", 28888},
  //                             fullstate_message_serialized.data(),
  //                             fullstate_message_serialized.size());

  //  EXPECT_TRUE(
  //      CompareMembers(node.GetMembers(), {{"node_a", "127.0.0.1", 27777},
  //                                         {"node_b", "127.0.0.1", 28888},
  //                                         {"node_c", "127.0.0.1", 29999},
  //                                         {"node_d", "127.0.0.1", 30000}}));
}

// compare
MATCHER_P2(VoidStrEq, data, size,
           negation
               ? "does not match"
               : "matches" +
                     std::string(reinterpret_cast<const char*>(data), size)) {
  return std::string(reinterpret_cast<const char*>(arg), size) ==
         std::string(reinterpret_cast<const char*>(data), size);
}

TEST(Membership, ClusterResponseToPullRequst) {
  membership::Membership node;
  membership::Config config;
  auto transport = std::make_shared<MockTransport>();
  config.SetHostMember("node_a", "127.0.0.1", 27777);

  membership::FullStateMessage message;
  message.InitAsFullStateMessage({{"node_a", "127.0.0.1", 27777, "127.0.0.1"}});
  auto message_serialized = message.SerializeToString();

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  std::string pull_request_message = "pull";
  auto result = transport->CallPullHandler(gossip::Address{"127.0.0.1", 28888},
                                           pull_request_message.data(),
                                           pull_request_message.size());
  EXPECT_EQ(result, std::vector<uint8_t>(message_serialized.begin(),
                                         message_serialized.end()));
}

TEST(Membership, EventSubcriptionWithMemberJoin) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  auto transport = std::make_shared<MockTransport>();

  node.Init(transport, config);

  auto subscriber = std::make_shared<MockSubscriber>();
  node.Subscribe(subscriber);

  EXPECT_CALL(*subscriber, Update);
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());

  SimulateReceivingUpMessage({"node2", "127.0.0.1", 28888}, transport);
}

TEST(Membership, EventSubcriptionWithMemberLeave) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  auto transport = std::make_shared<MockTransport>();

  node.Init(transport, config);

  auto subscriber = std::make_shared<MockSubscriber>();
  node.Subscribe(subscriber);

  EXPECT_CALL(*subscriber, Update).Times(0);
  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());

  SimulateReceivingDownMessage({"node2", "127.0.0.1", 27777}, transport);
  SimulateReceivingDownMessage({"node2", "127.0.0.1", 28888}, transport);
}

TEST(Membership, MemberLeaveFromSingleNodeCluster) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  config.SetRetransmitMultiplier(3);
  auto transport = std::make_shared<MockTransport>();

  membership::UpdateMessage message;
  message.InitAsDownMessage({"node1", "127.0.0.1", 27777}, 1);
  auto serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  SimulateReceivingDownMessage({"node2", "127.0.0.1", 28888}, transport);
  EXPECT_CALL(*transport, Gossip(_, payload, _)).Times(0);
  node.Init(transport, config);
}

TEST(Membership, MemberLeaveFromMultipleNodeCluster) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  config.SetRetransmitMultiplier(3);
  int retransmit_limit_two_node =
      config.GetRetransmitMultiplier() * ceil(log10(2 + 1));
  auto transport = std::make_shared<MockTransport>();

  membership::UpdateMessage message;
  message.InitAsDownMessage({"node2", "127.0.0.1", 28888}, 1);
  auto serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  SimulateReceivingUpMessage({"node2", "127.0.0.1", 28888}, transport);
  EXPECT_TRUE(CompareMembers(
      node.GetMembers(),
      {{"node1", "127.0.0.1", 27777}, {"node2", "127.0.0.1", 28888}}));

  EXPECT_CALL(*transport, Gossip(_, payload, _)).Times(0);
  SimulateReceivingDownMessage({"node2", "127.0.0.1", 28888}, transport);
  EXPECT_TRUE(
      CompareMembers(node.GetMembers(), {{"node1", "127.0.0.1", 27777}}));
}

TEST(Membership, NodeReceivingPingWithMoreMembers) {
  membership::Member node1{"node1", "127.0.0.1", 27777};
  membership::Member node2{"node2", "127.0.0.1", 28888};
  membership::Member node3{"node3", "127.0.0.1", 29999};
  membership::Member node4{"node4", "127.0.0.1", 30000};

  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  auto transport = std::make_shared<MockTransport>();

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  SimulateReceivingUpMessage(node2, transport);
  EXPECT_TRUE(CompareMembers(node.GetMembers(), {node1, node2}));

  std::map<membership::Member, int> members_in_ping;
  members_in_ping[node3] = 1;
  members_in_ping[node4] = 1;
  SimulateReceivingPingMessage(members_in_ping, transport);

  EXPECT_TRUE(CompareMembers(node.GetMembers(), {node1, node2, node3, node4}));
}

TEST(Membership, ShouldGetMembersWithHostNameWhenProvidingConfigWithHostName) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "localhost", 27777);
  auto transport = std::make_shared<MockTransport>();

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  auto members = node.GetMembers();
  EXPECT_EQ(members.size(), 1);
  auto member = members[0];
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
  EXPECT_EQ(member.GetHostName(), "localhost");
}

TEST(Membership,
     ShouldGetMembersWithIpAddressWhenProvidingConfigWithRealIpAddress) {
  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  auto transport = std::make_shared<MockTransport>();

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  auto members = node.GetMembers();
  EXPECT_EQ(members.size(), 1);
  auto member = members[0];
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
  EXPECT_EQ(member.GetHostName(), "127.0.0.1");
}

TEST(Membership, ShouldGetMembersWithIpAddressWhenReceivingUpUpdateMessage) {
  membership::Member node1{"node1", "127.0.0.1", 27777};
  membership::Member node2{"node2", "127.0.0.1", 28888, "localhost"};

  membership::Membership node;
  membership::Config config;
  config.SetHostMember("node1", "127.0.0.1", 27777);
  auto transport = std::make_shared<MockTransport>();

  EXPECT_CALL(*transport, Gossip).Times(AnyNumber());
  node.Init(transport, config);

  SimulateReceivingUpMessage(node2, transport);
  EXPECT_TRUE(CompareMembers(node.GetMembers(), {node1, node2}));
  auto members = node.GetMembers();
  for (const auto& member : members) {
    if (member == node1) {
      continue;
    }
    EXPECT_EQ(member.GetNodeName(), "node2");
    EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
    EXPECT_EQ(member.GetHostName(), "localhost");
  }
}
