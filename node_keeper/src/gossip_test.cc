/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <gmock/gmock.h>

using testing::NotNull;

TEST(Transport, ShouldReturnTransportIfCreateTransportWithAvailablePort) {
  gossip::Address udp = {"127.0.0.1", 5000};
  gossip::Address tcp = {"127.0.0.1", 5000};

  auto transport = gossip::CreateTransport(udp, tcp);

  EXPECT_THAT(transport, NotNull());
}

TEST(Transport, ShouldThrowErrorIfCreateTransportWithOccupiedPort) {
  gossip::Address udp = {"127.0.0.1", 5000};
  gossip::Address tcp = {"127.0.0.1", 5000};
  auto occupiedTransport = gossip::CreateTransport(udp, tcp);

  EXPECT_THROW(gossip::CreateTransport(udp, tcp), gossip::PortOccupied);
}
