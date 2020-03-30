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
    should_not_notify_other_nodes_when_deleting_a_node_with_config_specifying_no_notification_given_a_running_cluster) {
  auto node_a_ptr = std::make_unique<membership::Membership>();

  membership::Config config_a;
  config_a.AddHostMember("node_a", "127.0.0.1", 50000);
  config_a.SetLeaveWithoutNotification();
  std::shared_ptr<gossip::Transportable> transport_a =
      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
  node_a_ptr->Init(transport_a, config_a);

  membership::Membership node_b;
  membership::Config config_b;
  config_b.AddHostMember("node_a", "127.0.0.1", 50001);
  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
  std::shared_ptr<gossip::Transportable> transport_b =
      gossip::CreateTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
  node_b.Init(transport_b, config_b);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  ASSERT_EQ(2, node_a_ptr->GetMembers().size());
  ASSERT_EQ(2, node_b.GetMembers().size());

  transport_a.reset();
  node_a_ptr.reset();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_EQ(2, node_b.GetMembers().size());
}

// TEST(
//    FailureDetector,
//    should_detect_failed_node_when_deleting_a_node_unexpectedly_given_a_normally_running_cluster_of_two_nodes)
//    {
//
//  auto node_a_ptr = std::make_unique<membership::Membership>();
//  membership::Config config_a;
//  config_a.AddHostMember("node_a", "127.0.0.1", 50000);
//  config_a.SetLeaveWithoutNotification();
//  std::shared_ptr<gossip::Transportable> transport_a =
//      gossip::CreateTransport({"127.0.0.1", 50000}, {"127.0.0.1", 50000});
//  node_a_ptr->Init(transport_a, config_a);
//
//  membership::Membership node_b;
//  membership::Config config_b;
//  config_b.AddHostMember("node_a", "127.0.0.1", 50001);
//  config_b.AddOneSeedMember("node_a", "127.0.0.1", 50000);
//  std::shared_ptr<gossip::Transportable> transport_b =
//      gossip::CreateTransport({"127.0.0.1", 50001}, {"127.0.0.1", 50001});
//  node_b.Init(transport_b, config_b);
//
//  std::this_thread::sleep_for(std::chrono::seconds(5));
//
//  ASSERT_EQ(2, node_a_ptr->GetMembers().size());
//  ASSERT_EQ(2, node_b.GetMembers().size());
//
//  transport_a.reset();
//  node_a_ptr.reset();
//  std::this_thread::sleep_for(std::chrono::seconds(5));
//
//  EXPECT_EQ(1, node_b.GetMembers().size());
//}