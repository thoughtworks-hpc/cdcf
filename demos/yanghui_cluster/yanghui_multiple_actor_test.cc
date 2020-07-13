/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#define CAF_SUITE yanghui_triangle

#include "./simple_counter.h"
#include "./yanghui_simple_actor.h"
#include "caf/all.hpp"
#include "caf/test/dsl.hpp"
#include "caf/test/unit_test_impl.hpp"

namespace {

caf::behavior test_init_actor(caf::event_based_actor* self) { return {}; }

caf::behavior test_end_actor(caf::event_based_actor* self) {
  return {[](int result) {
    std::cout << "test complete, get result:" << result << std::endl;
  }};
}

struct yanghui_fixture : test_coordinator_fixture<config> {
  caf::actor counter_;
  caf::actor yanghui_count_path_;
  caf::actor yanghui_get_min_;
  caf::actor test_end_actor_;

  yanghui_fixture() {
    auto worker = sys.spawn(simple_counter);
    test_end_actor_ = sys.spawn(test_end_actor);
    std::cout << "spawn test_end_actor_:" << test_end_actor_.id() << std::endl;
    counter_ = caf::actor_cast<caf::actor>(worker);
    std::cout << "spawn worker_:" << counter_.id() << std::endl;
    yanghui_get_min_ =
        sys.spawn(yanghui_get_final_result, counter_, test_end_actor_);
    std::cout << "spawn yanghui_get_min_:" << yanghui_get_min_.id()
              << std::endl;
    yanghui_count_path_ =
        sys.spawn(yanghui_count_path, counter_, yanghui_get_min_);
    std::cout << "spawn yanghui_count_path_:" << yanghui_count_path_.id()
              << std::endl;

    // run();
  }
};

}  // namespace
std::vector<std::vector<int>> kYanghuiTestData = {{5}, {7, 8}, {2, 1, 4}};

CAF_TEST_FIXTURE_SCOPE(yanghui_trangle, yanghui_fixture)

CAF_TEST(yanghui interact) {
  // Spawn init

  auto test_init = sys.spawn(test_init_actor);
  auto sender = caf::actor_cast<caf::event_based_actor*>(test_init);
  sender->send(yanghui_count_path_, kYanghuiTestData);
  sched.run_once();

  // Test communication .
  expect((std::vector<std::vector<int>>),
         from(test_init).to(yanghui_count_path_).with(kYanghuiTestData));
  expect((int), from(yanghui_count_path_).to(yanghui_count_path_).with(1));
  expect((int, int, int), from(yanghui_count_path_).to(counter_).with(5, 7, 0));
  expect((int, int), from(counter_).to(yanghui_count_path_).with(12, 0));
  expect((int, int, int), from(yanghui_count_path_).to(counter_).with(5, 8, 1));
  expect((int, int), from(counter_).to(yanghui_count_path_).with(13, 1));
  expect((int), from(yanghui_count_path_).to(yanghui_count_path_).with(2));
  expect((int, int, int),
         from(yanghui_count_path_).to(counter_).with(12, 2, 0));
  expect((int, int), from(counter_).to(yanghui_count_path_).with(14, 0));
  expect((int, int, int, int),
         from(yanghui_count_path_).to(counter_).with(12, 13, 1, 1));
  expect((int, int), from(counter_).to(yanghui_count_path_).with(13, 1));
  // 1

  expect((int, int, int),
         from(yanghui_count_path_).to(counter_).with(13, 4, 2));
  expect((int, int), from(counter_).to(yanghui_count_path_).with(17, 2));
  expect((int), from(yanghui_count_path_).to(yanghui_count_path_).with(3));
  expect((std::vector<int>), from(yanghui_count_path_)
                                 .to(yanghui_get_min_)
                                 .with(std::vector<int>{14, 13, 17}));
  // 2

  NumberCompareData number_compare;
  number_compare.numbers = {14, 13, 17};
  number_compare.index = 0;
  expect((NumberCompareData),
         from(yanghui_get_min_).to(counter_).with(number_compare));
  expect((int, int), from(counter_).to(yanghui_get_min_).with(13, 0));
  expect(
      (std::vector<int>),
      from(yanghui_get_min_).to(yanghui_get_min_).with(std::vector<int>{13}));

  // final result
  expect((int), from(yanghui_get_min_).to(test_end_actor_).with(13));
}

CAF_TEST_FIXTURE_SCOPE_END()
