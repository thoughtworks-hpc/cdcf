/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#define CAF_SUITE yanghui

#include "./yanghui_actor.h"

#include "./FakeCounter.h"
#include "FakeRemoteCounter.h"
#include "caf/all.hpp"
#include "caf/test/dsl.hpp"
#include "caf/test/unit_test_impl.hpp"
#include "yanghui_config.h"

namespace {
struct fixture : test_coordinator_fixture<> {

  fixture() {
    auto worker_ = sys.spawn<typed_calculator>();
    worker = worker_;
    run();
  }

  decltype(sys.spawn<typed_calculator>()) worker;
};
}  // namespace

std::vector<std::vector<int>> kYanghuiTestData = {
    {5},
    {7, 8},
    {2, 1, 4}};

CAF_TEST_FIXTURE_SCOPE(yanghui_tests, fixture)

// Implements our first test.
CAF_TEST(divide) {
  //auto worker = sys.spawn<typed_calculator>();
//  FakeRemoteCounter* counter = new FakeRemoteCounter(worker, self);
//
//  auto yanghui_actor = sys.spawn(yanghui, (CounterInterface*)counter);
//  self->send(yanghui_actor, kYanghuiTestData);
//  sched.run_once();
//  self->request(worker, caf::infinite, 1, 2)
//      .receive(
//          [&](int remove_result) {
//            std::cout << remove_result << std::endl;
//          },
//          [&](caf::error& err) {
//          });


  expect((int, int), from(self).to(worker).with(_, 5, 7));

//  expect((int), from(worker).to(yanghui_actor).with(_, 12));
  disallow((int, int), from(self).to(worker).with(_, 5, 8));
}

CAF_TEST_FIXTURE_SCOPE_END()