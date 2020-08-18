/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <cdcf/logger.h>
#include <cdcf/router_pool.h>
#include <gtest/gtest.h>

#include "./router_pool_test_config.h"
#include "caf/io/all.hpp"

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

const size_t kDefaultActorSize = 3;

class RouterPoolTest : public ::testing::Test {
  void SetUp() override {
    std::string pool_name = "pool name";
    std::string pool_description = "pool description";
    std::string routee_name = "simple_calculator";
    auto routee_args = caf::make_message();
    auto routee_ifs = system_.message_types<simple_calculator>();
    size_t default_actor_num = kDefaultActorSize;
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
};

TEST_F(RouterPoolTest, should_return_default_size_3_when_add_a_new_node) {
  caf::scoped_actor self(system_);

  std::promise<bool> addNode_promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::add_atom::value, cdcf::router_pool::node_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](bool ret) { addNode_promise.set_value(ret); },
          [&](const caf::error& err) { addNode_promise.set_value(false); });
  bool addNode_promise_result = addNode_promise.get_future().get();
  EXPECT_EQ(true, addNode_promise_result);

  std::promise<size_t> promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) { promise.set_value(ret.size()); },
          [&](const caf::error& err) { promise.set_value(0); });
  int result = promise.get_future().get();

  EXPECT_EQ(3, result);
}

TEST_F(RouterPoolTest, should_return_4_when_add_an_new_actor) {
  caf::scoped_actor self(system_);

  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));
  self->send(pool_, caf::sys_atom::value, caf::update_atom::value,
             static_cast<size_t>(4));

  std::promise<size_t> promise_;

  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) { promise_.set_value(ret.size()); },
          [&](const caf::error& err) { promise_.set_value(0); });
  int result = promise_.get_future().get();
  EXPECT_EQ(4, result);
}

TEST_F(RouterPoolTest, should_return_2_when_delete_an_actor) {
  caf::scoped_actor self(system_);

  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));

  std::promise<bool> delete_request;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::update_atom::value, static_cast<size_t>(2))
      .receive([&](bool& ret) { delete_request.set_value(ret); },
               [&](const caf::error& err) { delete_request.set_value(false); });
  bool ans = delete_request.get_future().get();
  // risk
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  int result = 0;
  for (int i = 0; i < 10; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::promise<size_t> promise_;
    self->request(pool_, caf::infinite, caf::sys_atom::value,
                  caf::get_atom::value, cdcf::router_pool::actor_atom::value)
        .receive(
            [&](std::vector<caf::actor>& ret) {
              promise_.set_value(ret.size());
            },
            [&](const caf::error& err) { promise_.set_value(0); });
    result = promise_.get_future().get();
    std::cout << i << std::endl;
    if (result == 2) break;
  }
  EXPECT_EQ(true, ans);
  EXPECT_EQ(2, result);
}

TEST_F(RouterPoolTest, should_return_0_when_delete_a_node) {
  caf::scoped_actor self(system_);

  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));

  std::promise<bool> promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::delete_atom::value, cdcf::router_pool::node_atom::value,
                "", static_cast<uint16_t>(0))
      .receive([&](bool& ret) { promise.set_value(ret); },
               [&](const caf::error& err) { promise.set_value(false); });
  bool result = promise.get_future().get();
  EXPECT_EQ(true, result);

  std::promise<size_t> promise_;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) { promise_.set_value(ret.size()); },
          [&](const caf::error& err) { promise_.set_value(0); });
  int actor_size = promise_.get_future().get();
  EXPECT_EQ(0, actor_size);
}

TEST_F(RouterPoolTest, happy_path_send_one_add_msg) {
  caf::scoped_actor self(system_);

  std::promise<bool> addNode_promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::add_atom::value, cdcf::router_pool::node_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](bool ret) { addNode_promise.set_value(ret); },
          [&](const caf::error& err) { addNode_promise.set_value(false); });
  bool addNode_promise_result = addNode_promise.get_future().get();
  EXPECT_EQ(true, addNode_promise_result);

  std::promise<int> actor1_result_promise;
  self->request(pool_, caf::infinite, 1, 2)
      .receive(
          [&](int result, uint64_t actor_id) {
            actor1_result_promise.set_value(result);
          },
          [&](const caf::error& err) { actor1_result_promise.set_value(-1); });
  int actor1_result = actor1_result_promise.get_future().get();
  EXPECT_EQ(3, actor1_result);
}

TEST_F(RouterPoolTest, happy_path_send_three_add_msg) {
  caf::scoped_actor self(system_);

  std::promise<bool> addNode_promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::add_atom::value, cdcf::router_pool::node_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](bool ret) { addNode_promise.set_value(ret); },
          [&](const caf::error& err) { addNode_promise.set_value(false); });
  bool addNode_promise_result = addNode_promise.get_future().get();
  EXPECT_EQ(true, addNode_promise_result);

  std::promise<int> actor1_result_promise;
  std::promise<uint64_t> actor1_id_promise;
  self->request(pool_, caf::infinite, 3, 7)
      .receive(
          [&](int result, uint64_t actor_id) {
            actor1_result_promise.set_value(result);
            actor1_id_promise.set_value(actor_id);
          },
          [&](const caf::error& err) { actor1_result_promise.set_value(-1); });

  std::promise<int> actor2_result_promise;
  std::promise<uint64_t> actor2_id_promise;
  self->request(pool_, caf::infinite, 4, 8)
      .receive(
          [&](int result, uint64_t actor_id) {
            actor2_result_promise.set_value(result);
            actor2_id_promise.set_value(actor_id);
          },
          [&](const caf::error& err) { actor2_result_promise.set_value(-1); });

  std::promise<int> actor3_result_promise;
  std::promise<uint64_t> actor3_id_promise;
  self->request(pool_, caf::infinite, 5, 9)
      .receive(
          [&](int result, uint64_t actor_id) {
            actor3_result_promise.set_value(result);
            actor3_id_promise.set_value(actor_id);
          },
          [&](const caf::error& err) { actor3_result_promise.set_value(-1); });

  uint64_t actor1_id = actor1_id_promise.get_future().get();
  int actor1_result = actor1_result_promise.get_future().get();
  uint64_t actor2_id = actor2_id_promise.get_future().get();
  int actor2_result = actor2_result_promise.get_future().get();
  uint64_t actor3_id = actor3_id_promise.get_future().get();
  int actor3_result = actor3_result_promise.get_future().get();
  EXPECT_EQ(10, actor1_result);
  EXPECT_EQ(12, actor2_result);
  EXPECT_EQ(14, actor3_result);
  EXPECT_NE(actor1_id, actor2_id);
  EXPECT_NE(actor2_id, actor3_id);
  EXPECT_NE(actor1_id, actor3_id);
}

TEST_F(RouterPoolTest, shoule_return_same_result_if_delete_a_actor) {
  caf::scoped_actor self(system_);

  std::promise<bool> addNode_promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::add_atom::value, cdcf::router_pool::node_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](bool ret) { addNode_promise.set_value(ret); },
          [&](const caf::error& err) { addNode_promise.set_value(false); });
  bool addNode_promise_result = addNode_promise.get_future().get();
  EXPECT_EQ(true, addNode_promise_result);

  {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
      std::promise<int> result_promise;
      self->request(pool_, caf::infinite, 0, i)
          .receive(
              [&](int result, uint64_t actor_id) {
                result_promise.set_value(result);
              },
              [&](const caf::error& err) { result_promise.set_value(-1); });
      int actor_result = result_promise.get_future().get();
      sum += actor_result;
      EXPECT_EQ(0 + i, actor_result);
    }
    EXPECT_EQ(4950, sum);
  }

  std::promise<int> actor1_result_promise;
  std::promise<uint64_t> actor1_id_promise;
  self->request(pool_, caf::infinite, -1, -1)
      .receive(
          [&](int result, uint64_t actor_id) {
            actor1_result_promise.set_value(result);
            actor1_id_promise.set_value(actor_id);
          },
          [&](const caf::error& err) { actor1_result_promise.set_value(-1); });
  int actor1_result = actor1_result_promise.get_future().get();
  EXPECT_EQ(-1, actor1_result);

  {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
      std::promise<int> result_promise;
      self->request(pool_, caf::infinite, 0, i)
          .receive(
              [&](int result, uint64_t actor_id) {
                result_promise.set_value(result);
              },
              [&](const caf::error& err) { result_promise.set_value(-1); });
      int actor_result = result_promise.get_future().get();
      sum += actor_result;
      EXPECT_EQ(0 + i, actor_result);
    }
    EXPECT_EQ(4950, sum);
  }

  std::promise<std::vector<caf::actor>> getNode_promise;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value)
      .receive(
          [&](std::vector<caf::actor> ret) { getNode_promise.set_value(ret); },
          [&](const caf::error& err) { std::cout << "ERROR" << std::endl; });
  std::vector<caf::actor> ret = getNode_promise.get_future().get();
  EXPECT_EQ(2, ret.size());

  std::promise<size_t> promise_;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](std::vector<caf::actor>& ret) { promise_.set_value(ret.size()); },
          [&](const caf::error& err) { promise_.set_value(0); });
  int result = promise_.get_future().get();
  EXPECT_EQ(2, result);
}

TEST_F(RouterPoolTest, get_node_list) {
  caf::scoped_actor self(system_);

  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));

  std::promise<std::string> result;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::node_atom::value)
      .receive(
          [&](std::vector<std::string>& ret) {
            std::string info;
            for (auto& it : ret) {
              info += " ";
              info += it;
            }
            result.set_value(info);
          },
          [&](const caf::error& err) { result.set_value("ERROR"); });
  EXPECT_EQ(" ", result.get_future().get());
}

TEST_F(RouterPoolTest, get_actors_use_host_ports) {
  caf::scoped_actor self(system_);
  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, "", static_cast<uint16_t>(0));

  std::promise<size_t> promise_;
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value, "",
                static_cast<uint16_t>(0))
      .receive(
          [&](std::vector<caf::actor>& ret) { promise_.set_value(ret.size()); },
          [&](const caf::error& err) { promise_.set_value(0); });
  int result = promise_.get_future().get();
  EXPECT_EQ(3, result);
}
