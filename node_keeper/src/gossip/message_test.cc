/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip/message.h"

#include <gmock/gmock.h>

#include <vector>

using testing::Eq;

TEST(GossipMessage, ShouldDecodeMessageRight) {
  std::vector<uint8_t> buffer{0, 0, 0, 5, gossip::Message::Type::kPush,
                              1, 2, 3, 4, 5};
  gossip::Message message;

  auto consumed = message.Decode(buffer.data(), buffer.size());

  EXPECT_THAT(consumed, Eq(buffer.size()));
  EXPECT_THAT(message.Type(), Eq(gossip::Message::Type::kPush));
  EXPECT_THAT(message.Data(), Eq(std::vector<uint8_t>{1, 2, 3, 4, 5}));
}

TEST(GossipMessage, ShouldEncodeMessageRight) {
  std::vector<uint8_t> data{1, 2, 3, 4, 5};
  auto type = gossip::Message::Type::kPull;
  gossip::Message message(type, data.data(), data.size());

  auto buffer = message.Encode();

  EXPECT_THAT(buffer,
              Eq(std::vector<uint8_t>{0, 0, 0, 5, type, 1, 2, 3, 4, 5}));
}
