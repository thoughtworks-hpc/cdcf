/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "cdcf/actor_status_monitor.h"

#include <gmock/gmock.h>

caf::behavior Adder(caf::event_based_actor*) {
  return {[=](int a, int b) { return a + b; }};
}

class actor_status_monitor_test : public testing::Test {
 protected:
  void SetUp() override { test_actor_ = actor_system_.spawn(Adder); }

  void TearDown() override {
    caf::anon_send_exit(test_actor_, caf::exit_reason::kill);
  }

  caf::actor_system_config config_;
  caf::actor_system actor_system_{config_};
  caf::actor test_actor_;
};

TEST_F(actor_status_monitor_test, register_none_actor) {
  cdcf::actor_system::ActorStatusMonitor actorStatusMonitor(actor_system_);
  auto actor_infos = actorStatusMonitor.GetActorStatus();

  EXPECT_EQ(0, actor_infos.size());
}

TEST_F(actor_status_monitor_test, register_one_actor) {
  const std::string kTestActorName = "test_actor";
  const std::string kTestActorDescription = "for testing";
  cdcf::actor_system::ActorStatusMonitor actorStatusMonitor(actor_system_);
  actorStatusMonitor.RegisterActor(test_actor_, kTestActorName,
                                   kTestActorDescription);
  auto actor_infos = actorStatusMonitor.GetActorStatus();

  EXPECT_EQ(test_actor_.id(), actor_infos[0].id);
  EXPECT_EQ("for testing", actor_infos[0].description);
  EXPECT_EQ("test_actor", actor_infos[0].name);
}
