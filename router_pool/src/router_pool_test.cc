/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <gtest/gtest.h>
#include <logger.h>
#include <router_pool.h>

#include "./router_pool_test_config.h"
#include "caf/io/all.hpp"

caf::behavior test_init_actor(caf::event_based_actor* self) { return {}; }

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

class RouterPoolTest : public ::testing::Test {
  void SetUp() override {
    std::string pool_name = "pool name";
    std::string pool_description = "pool description";
    std::string routee_name = "simple_calculator";
    auto routee_args = caf::make_message();
    auto routee_ifs = system_.message_types<simple_calculator>();
    size_t default_actor_num = 3;
    auto policy = caf::actor_pool::round_robin();
    bool use_ssl = false;
    pool_ = system_.spawn<cdcf::router_pool::RouterPool>(
        system_, pool_name, pool_description, routee_name, routee_args,
        routee_ifs, default_actor_num, policy, use_ssl);
  }

  void TearDown() override {}

 public:
  router_config config_;
  caf::actor_system system_{config_};
  caf::actor pool_;
  caf::actor actor_1;
  caf::actor actor_2;
  std::promise<int> promise_;
};

TEST_F(RouterPoolTest, happy_path) {
  int error = 0;
  caf::scoped_actor self(system_);

  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));

  self->request(pool_, caf::infinite, 1, 2)
      .receive([&](int result) { promise_.set_value(result); },
               [&](const caf::error& err) { error = 1; });
  int result = promise_.get_future().get();

  EXPECT_EQ(3, result);
  EXPECT_EQ(0, error);
}
