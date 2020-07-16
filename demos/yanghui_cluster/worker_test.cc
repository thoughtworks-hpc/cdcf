/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

// 测试 杨辉三角中的 worker actor
#define CAF_SUITE actor
#include "./yanghui_config.h"
#include "caf/all.hpp"
#include "caf/test/unit_test_impl.hpp"

// 模版代码
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

CAF_TEST_FIXTURE_SCOPE(actor_tests, fixture)

CAF_TEST(simple actor test) {
  // spawn 杨辉的 worker actor， 待测 actor
  auto actor = sys.spawn<typed_calculator>();

  // 测试 Add 方法，判断返回值是否为两数之和
  self->request(actor, caf::infinite, 3, 4)
      .receive([=](int r) { CAF_CHECK(r == 7); },
               [&](caf::error& err) { CAF_FAIL(sys.render(err)); });

  std::vector<int> numbers = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  NumberCompareData send_data;
  send_data.numbers = numbers;

  // 测试 Compare 方法，判断返回值是否为最小元素
  self->request(actor, caf::infinite, send_data)
      .receive([=](int r) { CAF_CHECK(r == 0); },
               [&](caf::error& err) { CAF_FAIL(sys.render(err)); });
}

CAF_TEST_FIXTURE_SCOPE_END()
