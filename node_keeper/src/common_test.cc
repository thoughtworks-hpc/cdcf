/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "src/common.h"

#include <gmock/gmock.h>

TEST(SplitTest, shoud_split_correctly_given_one_space_seprate) {
  auto got = node_keeper::split("a b c", ' ');
  std::vector<std::string> want = {"a", "b", "c"};

  ASSERT_THAT(got, testing::Eq(want));
}

TEST(SplitTest, shoud_split_correctly_given_more_space_seprate) {
  auto got = node_keeper::split("a b  c   d", ' ');
  std::vector<std::string> want = {"a", "b", "c", "d"};

  ASSERT_THAT(got, testing::Eq(want));
}