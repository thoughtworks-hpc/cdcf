/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/channel.h"

#include <gmock/gmock.h>

using testing::Eq;

TEST(Channel, ShouldGetItemAfterPut) {
  node_keeper::Channel<int> channel;
  auto expect = 23;
  channel.Put(expect);
  int i = 0;
  auto result = channel.Get();

  ASSERT_TRUE(result.first);
  EXPECT_THAT(result.second, Eq(expect));
}

TEST(Channel, ShouldGetItemFalseAfterAllItemsWereGot) {
  node_keeper::Channel<int> channel;
  channel.Put(1);
  channel.Get();

  auto result = channel.Get(false);

  ASSERT_FALSE(result.first);
}

TEST(Channel, ShouldGetFalseAfterEmptyChannelClose) {
  node_keeper::Channel<int> channel;
  channel.Close();

  auto result = channel.Get();

  ASSERT_FALSE(result.first);
  EXPECT_TRUE(channel.IsClosed());
}

TEST(Channel, ShouldGetItemAfterClose) {
  node_keeper::Channel<int> channel;
  auto expect = 23;
  channel.Put(expect);
  channel.Close();

  auto result = channel.Get();

  ASSERT_TRUE(result.first);
  EXPECT_THAT(result.second, Eq(expect));
  EXPECT_TRUE(channel.IsClosed());
}

TEST(Channel, ShouldThrowExceptionWhilePutToClosedChannel) {
  node_keeper::Channel<int> channel;
  channel.Close();

  EXPECT_THROW(channel.Put(1), std::logic_error);
}
