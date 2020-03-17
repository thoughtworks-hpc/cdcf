/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip/message.h"

#include <gmock/gmock.h>

#include <iterator>
#include <vector>

using testing::Eq, testing::ContainerEq;

class GossipMessage : public testing::Test {
 protected:
  auto Encapsulate(enum gossip::Message::Type type,
                   const std::vector<uint8_t>& data) {
    /* FIXME: consider size more than 255 */
    const auto size_lowest_byte = static_cast<uint8_t>(data.size());
    auto buffer = std::vector<uint8_t>{0, 0, 0, size_lowest_byte, type};
    std::copy(data.begin(), data.end(), std::back_inserter(buffer));
    return buffer;
  }

  static const std::vector<uint8_t> kData;
};

const std::vector<uint8_t> GossipMessage::kData{1, 2, 3, 4, 5};

TEST_F(GossipMessage, ShouldDecodeMessageRight) {
  const auto type = gossip::Message::Type::kPush;
  const auto buffer = Encapsulate(type, kData);

  gossip::Message message;
  const auto consumed = message.Decode(buffer.data(), buffer.size());

  EXPECT_THAT(consumed, Eq(buffer.size()));
  EXPECT_THAT(message.Type(), Eq(type));
  EXPECT_THAT(message.Data(), ContainerEq(kData));
}

TEST_F(GossipMessage, ShouldEncodeMessageRight) {
  const auto type = gossip::Message::Type::kPull;
  gossip::Message message(type, kData.data(), kData.size());

  auto buffer = message.Encode();

  const auto expect = Encapsulate(type, kData);
  EXPECT_THAT(buffer, ContainerEq(expect));
}
