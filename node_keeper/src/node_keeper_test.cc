/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/node_keeper.h"

#include <gmock/gmock.h>

using testing::Eq;

TEST(NodeKeeper, should_create_new_cluster_without_seed) {
  node_keeper::Config config;
  config.parse_config({"--name=node0", "--host=127.0.0.1", "--port=5000"});
  const node_keeper::NodeKeeper keeper(config);

  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
}

TEST(NodeKeeper, should_create_new_cluster_if_node_is_primary_seed) {
  node_keeper::Config config;
  config.parse_config({"--name=node0", "--host=127.0.0.1", "--port=5000",
                       "--seeds=127.0.0.1:5000"});
  const node_keeper::NodeKeeper keeper(config);

  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
}

TEST(NodeKeeper, should_get_host_name_when_providing_config_with_host_name) {
  node_keeper::Config config;
  config.parse_config({"--name=node0", "--host=localhost", "--port=5000"});
  const node_keeper::NodeKeeper keeper(config);

  EXPECT_THAT(keeper.GetMembers().size(), Eq(1));
  EXPECT_THAT(keeper.GetMembers()[0].GetHostName(), Eq("localhost"));
  EXPECT_THAT(keeper.GetMembers()[0].GetIpAddress(), Eq("127.0.0.1"));
}

TEST(DISABLED_NodeKeeper, should_join_cluster_if_seed_node_is_secondary_seed) {
  node_keeper::Config config_primary;
  node_keeper::Config config_secondary;
  config_primary.parse_config({"--name=node0", "--host=127.0.0.1",
                               "--port=5000", "--seeds=127.0.0.1:5000"});
  config_secondary.parse_config({"--name=node0", "--host=127.0.0.1",
                                 "--port=5001", "--seeds=127.0.0.1:5001"});
  const node_keeper::NodeKeeper primary(config_primary);

  const node_keeper::NodeKeeper secondary(config_secondary);

  EXPECT_THAT(secondary.GetMembers().size(), Eq(2));
}
