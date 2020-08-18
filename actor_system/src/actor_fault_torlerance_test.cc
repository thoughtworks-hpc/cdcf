/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <gtest/gtest.h>

#include "cdcf/actor_guard.h"
#include "cdcf/actor_union.h"

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

caf::behavior calculator_fun(caf::event_based_actor* self) {
  return {
      [=](add_atom, int a, int b) -> int {
        CDCF_LOGGER_INFO(
            "received add_atom task from remote node. input a:{} b:{}", a, b);
        return a + b;
      },
      [=](sub_atom, int a, int b) -> int {
        CDCF_LOGGER_INFO(
            "received sub_atom task from remote node. input a:{} b:{}", a, b);

        return a - b;
      }};
}

caf::behavior calculator_error(caf::event_based_actor* self) {
  return {[=](add_atom, int a, int b) {
    caf::error error = caf::make_error(caf::sec::bad_function_call);
    return error;
  }};
}

class ActorUnionTest : public ::testing::Test {
  void SetUp() override {
    calculator1_ = system_.spawn(calculator_fun);
    calculator2_ = system_.spawn(calculator_fun);
    error_actor_ = system_.spawn(calculator_error);
  }

  void TearDown() override {}

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor calculator1_;
  caf::actor calculator2_;
  caf::actor error_actor_;
  cdcf::actor_system::ActorUnion actorUnion_{system_,
                                             caf::actor_pool::round_robin()};
};

TEST_F(ActorUnionTest, happy_path) {
  actorUnion_.AddActor(calculator1_);
  int result = 0;
  bool error = false;

  std::promise<int> promise;
  actorUnion_.SendAndReceive(
      [&](int return_value) { promise.set_value(return_value); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(8, result);
  EXPECT_EQ(false, error);
}

TEST_F(ActorUnionTest, has_one_exit_actor) {
  actorUnion_.AddActor(calculator2_);
  actorUnion_.AddActor(calculator1_);

  // simulate actor fault
  caf::anon_send_exit(calculator2_, caf::exit_reason::kill);

  int result = 0;
  bool error = false;

  std::promise<int> promise;
  actorUnion_.SendAndReceive(
      [&](int return_value) { promise.set_value(return_value); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(8, result);
  EXPECT_EQ(false, error);
}

TEST_F(ActorUnionTest, has_one_system_error_actor) {
  actorUnion_.AddActor(error_actor_);
  actorUnion_.AddActor(calculator1_);

  int result = 0;
  bool error = false;

  std::promise<int> promise;
  actorUnion_.SendAndReceive(
      [&](int return_value) { promise.set_value(return_value); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(8, result);
  EXPECT_EQ(false, error);
}

TEST_F(ActorUnionTest, all_error_actor) {
  actorUnion_.AddActor(calculator2_);
  actorUnion_.AddActor(calculator1_);

  // simulate all actor fault
  caf::anon_send_exit(calculator1_, caf::exit_reason::kill);
  caf::anon_send_exit(calculator2_, caf::exit_reason::kill);

  int result = 0;
  bool error = false;

  std::promise<int> promise;
  actorUnion_.SendAndReceive(
      [&](int return_value) { promise.set_value(return_value); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(0, result);
  EXPECT_EQ(true, error);
}

class ActorGuardTest : public ::testing::Test {
  void SetUp() override {
    calculator1_ = system_.spawn(calculator_fun);
    calculator2_ = system_.spawn(calculator_fun);
    error_actor_ = system_.spawn(calculator_error);
  }

  void TearDown() override {}

 public:
  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::actor calculator1_;
  caf::actor calculator2_;
  caf::actor error_actor_;
};

TEST_F(ActorGuardTest, happy_path) {
  std::promise<int> promise;
  int result = 0;
  bool error = false;

  ActorGuard actor_guard(
      calculator1_,
      [=](std::atomic<bool>& active) -> caf::actor {
        return system_.spawn(calculator_fun);
      },
      system_);

  actor_guard.SendAndReceive(
      [&](int return_vale) { promise.set_value(return_vale); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(result, 8);
  EXPECT_EQ(false, error);
}

TEST_F(ActorGuardTest, init_error_actor) {
  std::promise<int> promise;
  int result = 0;
  bool error = false;

  ActorGuard actor_guard(
      error_actor_,
      [=](std::atomic<bool>& active) -> caf::actor {
        return system_.spawn(calculator_fun);
      },
      system_);

  actor_guard.SendAndReceive(
      [&](int return_vale) { promise.set_value(return_vale); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(result, 8);
  EXPECT_EQ(false, error);
}

TEST_F(ActorGuardTest, actor_exit) {
  std::promise<int> promise;
  int result = 0;
  bool error = false;

  ActorGuard actor_guard(
      calculator1_,
      [=](std::atomic<bool>& active) -> caf::actor {
        return system_.spawn(calculator_fun);
      },
      system_);

  caf::anon_send_exit(calculator1_, caf::exit_reason::kill);

  actor_guard.SendAndReceive(
      [&](int return_vale) { promise.set_value(return_vale); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(result, 8);
  EXPECT_EQ(false, error);
}

TEST_F(ActorGuardTest, actor_restart_failed) {
  std::promise<int> promise;
  int result = 0;
  bool error = false;

  ActorGuard actor_guard(
      calculator1_,
      [=](std::atomic<bool>& active) -> caf::actor {
        active = false;
        return system_.spawn(calculator_fun);
      },
      system_);

  caf::anon_send_exit(calculator1_, caf::exit_reason::kill);

  actor_guard.SendAndReceive(
      [&](int return_vale) { promise.set_value(return_vale); },
      [&](const caf::error&) {
        error = true;
        promise.set_value(0);
      },
      add_atom::value, 3, 5);

  result = promise.get_future().get();

  EXPECT_EQ(result, 0);
  EXPECT_EQ(true, error);
}
