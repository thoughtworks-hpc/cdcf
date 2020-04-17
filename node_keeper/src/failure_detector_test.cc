/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include <chrono>
#include <memory>

#include "gtest/gtest.h"
#include "src/gossip.h"
#include "src/membership.h"

TEST(
    FailureDetector,
    should_detect_node_a_fail_from_node_b_when_node_a_leave_without_notify_given_a_running_cluster_of_nodes_a_b) {
  auto node_a_ptr = std::make_unique<membership::Membership>();
  membership::Config config_a;
  config_a.AddHostMember("node_a", "127.0.0.1", 50000);
  config_a.SetLeaveWithoutNotification();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a_ptr->Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.AddHostMember("node_b", "127.0.0.1", 50001);
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

TEST(
    FailureDetector,
    should_aware_node_a_fail_in_c_when_node_a_leave_without_notify_given_a_cluster_of_a_b_c_nodes_and_node_c_has_failure_detector_turned_off) {
  auto node_a_ptr = std::make_unique<membership::Membership>();
  membership::Config config_a;
  config_a.AddHostMember("node_a", "127.0.0.1", 50000);
  config_a.SetLeaveWithoutNotification();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a_ptr->Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.AddHostMember("node_b", "127.0.0.1", 50001);
  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
  config_b.SetFailureDetectorIntervalInMilliSeconds(500);
  std::shared_ptr<gossip::Transportable> transport_b =
      gossip::CreateTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
  node_b.Init(transport_b, config_b);

  membership::Membership node_c;
  membership::Config config_c;
  config_c.AddHostMember("node_c", "127.0.0.1", 50002);
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
