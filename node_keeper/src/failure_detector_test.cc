/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <chrono>
#include <memory>

#include "gtest/gtest.h"
#include "src/gossip.h"
#include "src/membership.h"
// anti clang-format sort
#include "src/fake_gossip.h"

/*
 * given a cluster of two nodes A B running normally
 * when A leaves without notifying B
 * then B will detect A fails
 */
TEST(FailureDetector,
     should_detect_node_a_fail_when_node_a_leave_without_notify) {
  auto node_a_ptr = std::make_unique<membership::Membership>();
  membership::Config config_a;
  config_a.SetHostMember("node_a", "127.0.0.1", 50000);
  config_a.EnableLeaveWithoutNotification();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a_ptr->Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.SetHostMember("node_b", "127.0.0.1", 50001);
  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
  config_b.SetFailureDetectorIntervalInMilliSeconds(500);
  std::shared_ptr<gossip::Transportable> transport_b =
      gossip::CreateTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
  node_b.Init(transport_b, config_b);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  ASSERT_EQ(2, node_a_ptr->GetMembers().size());
  ASSERT_EQ(2, node_b.GetMembers().size());

  // node a leave
  transport_a.reset();
  node_a_ptr.reset();

  std::this_thread::sleep_for(std::chrono::seconds(2));

  EXPECT_EQ(1, node_b.GetMembers().size());
  EXPECT_EQ(1, node_b.GetSuspects().size());
  auto expected_suspects =
      std::vector<membership::Member>{{"node_a", "127.0.0.1", 50000}};
  EXPECT_EQ(expected_suspects, node_b.GetSuspects());
}

/*
 * given a cluster of three nodes A B C running normally and C has failure
 * detector turned off when A leaves without notifying B and C then C will
 * become aware of A's failure
 */
TEST(FailureDetector,
     should_aware_node_a_fail_in_c_when_node_a_leave_without_notify) {
  auto node_a_ptr = std::make_unique<membership::Membership>();
  membership::Config config_a;
  config_a.SetHostMember("node_a", "127.0.0.1", 50000);
  config_a.EnableLeaveWithoutNotification();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a_ptr->Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.SetHostMember("node_b", "127.0.0.1", 50001);
  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
  config_b.SetFailureDetectorIntervalInMilliSeconds(500);
  std::shared_ptr<gossip::Transportable> transport_b =
      gossip::CreateTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
  node_b.Init(transport_b, config_b);

  membership::Membership node_c;
  membership::Config config_c;
  config_c.SetHostMember("node_c", "127.0.0.1", 50002);
  config_c.AddOneSeedMember("node_b", "127.0.0.1", 50001);
  config_c.SetFailureDetectorIntervalInMilliSeconds(500);
  config_c.SetFailureDetectorOff();
  std::shared_ptr<gossip::Transportable> transport_c =
      gossip::CreateTransport({"127.0.0.1", 50002}, {"127.0.0.1", 50002});
  node_c.Init(transport_c, config_c);

  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "members in node a: " << std::endl;
  for (const auto& member : node_a_ptr->GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node b: " << std::endl;
  for (const auto& member : node_b.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node c: " << std::endl;
  for (const auto& member : node_c.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  EXPECT_EQ(3, node_a_ptr->GetMembers().size());
  EXPECT_EQ(3, node_b.GetMembers().size());
  EXPECT_EQ(3, node_c.GetMembers().size());

  // node a leave
  transport_a.reset();
  node_a_ptr.reset();

  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "members in node b: " << std::endl;
  for (const auto& member : node_b.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node c: " << std::endl;
  for (const auto& member : node_c.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "suspects in node b: " << std::endl;
  for (const auto& member : node_b.GetSuspects()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "suspects in node c: " << std::endl;
  for (const auto& member : node_c.GetSuspects()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  EXPECT_EQ(2, node_b.GetMembers().size());
  EXPECT_EQ(1, node_b.GetSuspects().size());
  auto expected_suspects =
      std::vector<membership::Member>{{"node_a", "127.0.0.1", 50000}};
  EXPECT_EQ(expected_suspects, node_b.GetSuspects());
  EXPECT_EQ(2, node_c.GetMembers().size());
  EXPECT_EQ(1, node_c.GetSuspects().size());
  EXPECT_EQ(expected_suspects, node_c.GetSuspects());
}

/*
 * given a cluster of three nodes A B and C running normally
 * when A and B become unreachable over network and A C, B C still reachable
 * then B should not suspect A
 */
TEST(FailureDetector, should_not_suspect_a_when_a_and_b_become_unreachable) {
  membership::Membership node_a;
  membership::Config config_a;
  config_a.SetHostMember("node_a", "127.0.0.1", 50000);
  config_a.SetFailureDetectorIntervalInMilliSeconds(500);
  // config_.EnableRaleyPing();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a.Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.SetHostMember("node_b", "127.0.0.1", 50001);
  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
  config_b.SetFailureDetectorIntervalInMilliSeconds(500);
  std::shared_ptr<UnreachableTransport> transport_b =
      CreateUnreachableTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
  node_b.Init(transport_b, config_b);

  membership::Membership node_c;
  membership::Config config_c;
  config_c.SetHostMember("node_c", "127.0.0.1", 50002);
  config_c.AddOneSeedMember("node_b", "127.0.0.1", 50001);
  config_c.SetFailureDetectorIntervalInMilliSeconds(500);
  config_c.SetFailureDetectorOff();
  std::shared_ptr<gossip::Transportable> transport_c =
      gossip::CreateTransport({"127.0.0.1", 50002}, {"127.0.0.1", 50002});
  node_c.Init(transport_c, config_c);

  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "members in node a: " << std::endl;
  for (const auto& member : node_a.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node b: " << std::endl;
  for (const auto& member : node_b.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node c: " << std::endl;
  for (const auto& member : node_c.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  EXPECT_EQ(3, node_a.GetMembers().size());
  EXPECT_EQ(3, node_b.GetMembers().size());
  EXPECT_EQ(3, node_c.GetMembers().size());

  // A and B become unreachable
  transport_b->MakeUnreachableTo(gossip::Address{"127.0.0.1", 50000});
  std::cout << "disconnect A and B" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(3));

  EXPECT_EQ(3, node_a.GetMembers().size());
  EXPECT_EQ(3, node_b.GetMembers().size());
  EXPECT_EQ(3, node_c.GetMembers().size());

  std::cout << "members in node a: " << std::endl;
  for (const auto& member : node_a.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node b: " << std::endl;
  for (const auto& member : node_b.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node c: " << std::endl;
  for (const auto& member : node_c.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  // C and B become unreachable
  transport_b->MakeUnreachableTo(gossip::Address{"127.0.0.1", 50002});
  std::cout << "disconnect B and C" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(5));

  EXPECT_EQ(1, node_b.GetMembers().size());

  std::cout << "members in node a: " << std::endl;
  for (const auto& member : node_a.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node b: " << std::endl;
  for (const auto& member : node_b.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;

  std::cout << "members in node c: " << std::endl;
  for (const auto& member : node_c.GetMembers()) {
    std::cout << member.GetPort() << ", ";
  }
  std::cout << std::endl;
}
