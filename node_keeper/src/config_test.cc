/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/config.h"

#include <gmock/gmock.h>

#include <array>

using testing::ContainerEq, testing::Eq;

TEST(Config, ShouldParseSeedsRight) {
  std::vector<gossip::Address> seeds{
      {"127.0.0.1", 3000}, {"127.0.0.2", 4000}, {"127.0.0.3", 5000}};
  std::string seeds_arg{"--seeds="};
  for (size_t i = 0; i < seeds.size(); ++i) {
    if (i != 0) {
      seeds_arg += ',';
    }
    seeds_arg += seeds[i].host + ':' + std::to_string(seeds[i].port);
  }
  std::array<char*, 2> args = {const_cast<char*>("app"),
                               const_cast<char*>(seeds_arg.c_str())};

  node_keeper::Config config;
  auto result = config.parse_config(args.size(), &args[0], "cdcf-default.ini");

  ASSERT_THAT(result, Eq(CDCFConfig::RetValue::kSuccess));
  auto actual = config.GetSeeds();
  EXPECT_THAT(actual, ContainerEq(seeds));
}
