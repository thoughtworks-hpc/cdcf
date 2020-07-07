/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#define CAF_SUITE yanghui

#include "./yanghui_actor.h"

#include "./FakeCounter.h"
#include "caf/all.hpp"
#include "caf/test/dsl.hpp"
#include "caf/test/unit_test_impl.hpp"

namespace {
struct fixture {
  caf::actor_system_config cfg;
  caf::actor_system sys;
  caf::scoped_actor self;
  fixture() : sys(cfg), self(sys) {
    // nop
  }
};
}  // namespace

std::vector<std::vector<int>> kYanghuiTestData = {
    {5},
    {7, 8},
    {2, 1, 4},
    {4, 2, 6, 1},
    {2, 7, 3, 4, 5},
    {2, 3, 7, 6, 8, 3},
    {2, 3, 4, 4, 2, 7, 7},
    {8, 4, 4, 3, 4, 5, 6, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6}};

CAF_TEST_FIXTURE_SCOPE(yanghui_tests, fixture)

// Implements our first test.
CAF_TEST(divide) {
  CounterInterface* counter = new FakeCounter();
  auto yanghui_actor = sys.spawn(yanghui, counter);
  self->request(yanghui_actor, caf::infinite, kYanghuiTestData)
      .receive([=](int r) { CAF_CHECK(r == 42); },
               [&](caf::error& err) { CAF_FAIL(sys.render(err)); });
}

CAF_TEST_FIXTURE_SCOPE_END()