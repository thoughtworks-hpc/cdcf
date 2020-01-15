/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include <gmock/gmock.h>

#include <vector>

#include "../include/membership.h"

TEST(Test, Case) { EXPECT_THAT(1, ::testing::Eq(1)); }

TEST(Membership, CreateOneMember) {
  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(member.GetNodeName(), "node1");
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
  EXPECT_EQ(member.GetPort(), 27777);
}

TEST(Membership, CreateLocalNode) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  my_membership.Init("node1", "127.0.0.1", 27777);

  std::vector<membership::Member> members = my_membership.GetMembers();
  EXPECT_EQ(members.size(), 1);
  EXPECT_EQ(members[0], member);
}