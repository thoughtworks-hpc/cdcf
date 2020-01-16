/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include <vector>

#include "../include/gossip.h"
#include "../include/membership.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "include/mock_gossip.h"

using namespace testing;

// Member

bool CompareMembers(const std::vector<membership::Member> &lhs,
                    const std::vector<membership::Member> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (int i = lhs.size(); i < lhs.size(); ++i) {
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
  config.AddHostMember("node1", "127.0.0.1", 27777);

  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(config.GetHostMember(), member);
}

TEST(Membership, ConfigWithSeedMember) {
  membership::Config config;
  config.AddOneSeedMember("node1", "127.0.0.1", 27777);
  std::vector<membership::Member> seed_members = config.GetSeedMembers();

  membership::Member seed_member1("node1", "127.0.0.1", 27777);
  std::vector<membership::Member> seed_members_compare;
  seed_members_compare.push_back(seed_member1);

  EXPECT_TRUE(CompareMembers(seed_members, seed_members_compare));
}

TEST(Membership, ConfigWithTransport) {
  MockTransport transport;
  membership::Config config;

  config.AddTransport(&transport);
  EXPECT_EQ(config.GetTransport(), &transport);
}

// Membership
TEST(Membership, CreateHostMember) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;
  config.AddHostMember("node1", "127.0.0.1", 27777);

  my_membership.Init(config);

  std::vector<membership::Member> members = my_membership.GetMembers();
  EXPECT_EQ(members.size(), 1);
  EXPECT_EQ(members[0], member);
}

TEST(Membership, CreateHostMemberWithEmptyConfig) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;

  EXPECT_EQ(my_membership.Init(config),
            membership::MEMBERSHIP_INIT_HOSTMEMBER_EMPTY);
}