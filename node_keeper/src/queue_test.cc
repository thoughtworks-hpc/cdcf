/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/queue.h"

#include <chrono>
#include <future>
#include <set>
#include <string>

#include "gtest/gtest.h"

TEST(Queue, OneFunctorCalledOneTime) {
  queue::TimedFunctorQueue queue(std::chrono::milliseconds(500));

  std::promise<std::string> promise;
  auto future = promise.get_future();
  auto functor = [&promise]() { promise.set_value("hello"); };

  queue.Push(functor, 1);

  auto status = future.wait_for(std::chrono::seconds(1));

  ASSERT_EQ(status, std::future_status::ready);

  ASSERT_EQ(future.get(), "hello");
}

TEST(Queue, OneFunctorCalledThreeTimes) {
  queue::TimedFunctorQueue queue(std::chrono::milliseconds(500));

  std::multiset<int> store;
  std::mutex store_mut;
  std::condition_variable store_cond;
  auto functor = [&]() {
    std::lock_guard lk(store_mut);
    store.insert(1);
    store_cond.notify_one();
  };

  queue.Push(functor, 3);

  std::unique_lock lk(store_mut);
  ASSERT_TRUE(store_cond.wait_for(lk, std::chrono::seconds(5),
                                  [&]() { return store.size() == 3; }));
}

TEST(Queue, TwoFunctorCalledOneTime) {
  queue::TimedFunctorQueue queue(std::chrono::milliseconds(500));

  std::promise<std::string> promise1;
  auto future1 = promise1.get_future();
  auto functor1 = [&promise1]() { promise1.set_value("hello"); };
  std::promise<std::string> promise2;
  auto future2 = promise2.get_future();
  auto functor2 = [&promise2]() { promise2.set_value("world"); };

  queue.Push(functor1, 1);
  queue.Push(functor2, 1);

  auto status = future1.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready);
  status = future2.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready);

  ASSERT_EQ(future1.get(), "hello");
  ASSERT_EQ(future2.get(), "world");
}

TEST(Queue, OneFunctorCalledZeroTime) {
  queue::TimedFunctorQueue queue(std::chrono::milliseconds(500));

  std::promise<std::string> promise;
  auto future = promise.get_future();
  auto functor = [&promise]() { promise.set_value("hello"); };

  queue.Push(functor, 0);

  auto status = future.wait_for(std::chrono::seconds(1));

  ASSERT_EQ(status, std::future_status::timeout);
}
