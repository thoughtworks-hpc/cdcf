/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/node_keeper.h"

#include <gmock/gmock.h>

using testing::Eq;

TEST(NodeKeeper, should_create_new_cluster_without_seed) {
  const node_keeper::NodeKeeper keeper("node0", {"127.0.0.1", 5000});

  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
}

TEST(NodeKeeper, should_create_new_cluster_if_node_is_primary_seed) {
  const std::vector<gossip::Address> seeds{{"127.0.0.1", 5000}};
  const node_keeper::NodeKeeper keeper("node0", {"127.0.0.1", 5000}, seeds);

  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
}

TEST(DISABLED_NodeKeeper, should_join_cluster_if_seed_node_is_secondary_seed) {
  const std::vector<gossip::Address> seeds{{"127.0.0.1", 5000},
                                           {"127.0.0.2", 5001}};
  const node_keeper::NodeKeeper primary("node0", seeds[0], seeds);

  const node_keeper::NodeKeeper secondary("node0", seeds[1], seeds);

  EXPECT_THAT(secondary.GetMembers().size(), Eq(2));
}
