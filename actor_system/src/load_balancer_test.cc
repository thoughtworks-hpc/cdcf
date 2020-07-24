/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include <actor_system/load_balancer.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <future>
#include <iterator>

using testing::Eq, testing::NotNull;

class State {
 public:
  std::atomic<size_t> count{0};

  void Lock() {
    start_promise_.set_value();
    stop_promise_.get_future().get();
  }
  void Unlock() try { stop_promise_.set_value(); } catch (...) {
  }
  void WaitForLocked() { start_promise_.get_future().get(); }

 private:
  std::promise<void> start_promise_;
  std::promise<void> stop_promise_;
};

using factorial_atom = caf::atom_constant<caf::atom("factorial")>;
using lock_atom = caf::atom_constant<caf::atom("lock")>;
using count_atom = caf::atom_constant<caf::atom("count")>;
using Worker = caf::stateful_actor<State>;
caf::behavior adder(Worker* self) {
  return {[=](factorial_atom, size_t x) {
            ++self->state.count;
            size_t result = 1;
            for (size_t i = 1; i <= x; ++i) {
              result *= i;
            }
            return result;
          },
          [=](count_atom) -> size_t { return self->state.count; },
          [=](lock_atom) { self->state.Lock(); }};
}

MATCHER_P(ExecutedTimes, times,
          "actor executed " + std::to_string(times) + " time(s)") {
  auto func = caf::make_function_view(arg);
  auto message = *func(count_atom::value);
  auto count = *reinterpret_cast<const size_t*>(message.get(0));
  *result_listener << "it's executed " << count << " time(s)";
  return count == times;
}

MATCHER_P2(AllExecutedTimesNear, times, error,
           "all of actor executed " + std::to_string(times) + " time(s)") {
  std::vector<size_t> counts(arg.size());
  std::transform(arg.begin(), arg.end(), counts.begin(), [](auto& worker) {
    auto func = make_function_view(worker);
    auto message = *func(count_atom::value);
    return *reinterpret_cast<const size_t*>(message.get(0));
  });

  std::stringstream ss;
  std::copy(counts.begin(), counts.end(),
            std::ostream_iterator<size_t>(ss, ","));
  *result_listener << "they're executed {" << ss.str() << "} time(s)";
  auto every_as_expected =
      std::all_of(counts.begin(), counts.end(), [=](auto& i) {
        return static_cast<uint64_t>(std::abs(static_cast<intmax_t>(times - i))) <= error;
      });
  size_t init_sum = 0;
  auto total = std::accumulate(counts.begin(), counts.end(), init_sum);
  auto total_as_expected = total == times * arg.size();
  return every_as_expected && total_as_expected;
}

class LoadBalancerTest : public ::testing::Test {
 protected:
  void Prepare(size_t workers_count, size_t threshold = 10) {
    auto policy = cdcf::load_balancer::policy::MinLoad(threshold);
    balancer_ = cdcf::load_balancer::Router::Make(&context, std::move(policy));
    std::generate_n(std::back_inserter(workers_), workers_count,
                    [&]() { return system_.spawn(adder); });
    for (const auto& worker : workers_) {
      caf::anon_send(balancer_, caf::sys_atom::value, caf::put_atom::value,
                     worker);
    }
  }

  void TearDown() override {
    if (should_cleanup_) {
      for (const auto& actor : workers_) {
        caf::actor_cast<Worker*>(actor)->state.Unlock();
      }
    }
  }

  caf::actor_system_config config_;
  caf::actor_system system_{config_};
  caf::scoped_execution_unit context{&system_};
  caf::actor balancer_;
  std::vector<caf::actor> workers_;
  bool should_cleanup_{true};
};

TEST_F(LoadBalancerTest, should_work_behind_load_balancer) {
  Prepare(1);

  auto message =
      make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_THAT(message->get_as<size_t>(0), Eq(2));
  EXPECT_THAT(workers_[0], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_reply_empty_message_without_worker) {
  Prepare(0);

  auto result = make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_TRUE(result);
  EXPECT_THAT(result->size(), Eq(0));
}

TEST_F(LoadBalancerTest, should_route_evenly_with_function_view) {
  Prepare(2);

  make_function_view(balancer_)(factorial_atom::value, size_t{2});
  make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_THAT(workers_[0], ExecutedTimes(1));
  EXPECT_THAT(workers_[1], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_route_evenly_with_request_receive) {
  Prepare(2);

  caf::scoped_actor self{system_};
  for (const auto& worker : workers_) {
    self->request(balancer_, caf::infinite, factorial_atom::value, size_t{2})
        .receive([&](int result) {}, [&](caf::error& error) {});
  }

  EXPECT_THAT(workers_[0], ExecutedTimes(1));
  EXPECT_THAT(workers_[1], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_route_evenly_with_request_then) {
  Prepare(4);

  auto async = [&](caf::event_based_actor* self, caf::actor balancer) {
    auto dummy = []() {};
    for (const auto& worker : workers_) {
      self->request(balancer, caf::infinite, factorial_atom::value, size_t{2})
          .then(dummy);
    }
  };
  caf::scoped_actor self{system_};
  auto actor = self->spawn(async, balancer_);
  self->wait_for(actor);

  EXPECT_THAT(workers_[0], ExecutedTimes(1));
  EXPECT_THAT(workers_[1], ExecutedTimes(1));
  EXPECT_THAT(workers_[2], ExecutedTimes(1));
  EXPECT_THAT(workers_[3], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_route_to_first_worker_while_all_are_idle) {
  Prepare(4);

  make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_THAT(workers_[0], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_route_to_second_worker_while_first_is_busy) {
  Prepare(2);
  caf::anon_send(balancer_, lock_atom::value);
  caf::actor_cast<Worker*>(workers_[0])->state.WaitForLocked();

  make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_THAT(workers_[1], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest,
       should_route_to_last_worker_while_all_priors_are_busy) {
  Prepare(4);
  std::for_each(workers_.begin(), workers_.end() - 1, [&](auto worker) {
    caf::anon_send(balancer_, lock_atom::value);
    caf::actor_cast<Worker*>(worker)->state.WaitForLocked();
  });

  make_function_view(balancer_)(factorial_atom::value, size_t{2});

  EXPECT_THAT(workers_[3], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_route_to_first_worker_while_all_are_busy) {
  Prepare(4);
  for (auto& worker : workers_) {
    caf::anon_send(balancer_, lock_atom::value);
    caf::actor_cast<Worker*>(worker)->state.WaitForLocked();
  }

  caf::anon_send(balancer_, factorial_atom::value, size_t{2});
  caf::actor_cast<Worker*>(workers_[0])->state.Unlock();

  EXPECT_THAT(workers_[0], ExecutedTimes(1));
}

TEST_F(LoadBalancerTest, should_exit_workers_while_send_exit_to_balancer) {
  Prepare(4);
  const auto reason = caf::exit_reason::remote_link_unreachable;
  std::promise<caf::exit_reason> promise;
  workers_[0]->attach_functor([&](const caf::error& reason) {
    promise.set_value(caf::exit_reason{reason.code()});
  });

  caf::anon_send_exit(balancer_, reason);

  auto actual = promise.get_future().get();
  EXPECT_THAT(actual, Eq(reason));
  should_cleanup_ = false;
}

TEST_F(LoadBalancerTest, should_roughly_route_even_under_throughput_load) {
  constexpr size_t concurrent = 4;
  constexpr size_t times = 200;
  constexpr size_t x = 3'300'000;
  constexpr size_t load_threshold = 5;
  Prepare(concurrent, load_threshold);

  auto async = [=](caf::event_based_actor* self, caf::actor balancer) {
    auto dummy = [](size_t) {};
    for (size_t i = 0; i < times; ++i) {
      self->request(balancer, caf::infinite, factorial_atom::value, x)
          .then(dummy);
    }
  };
  caf::scoped_actor self{system_};
  auto actor = self->spawn(async, balancer_);
  self->wait_for(actor);

  EXPECT_THAT(workers_, AllExecutedTimesNear(times / concurrent,
                                             load_threshold * concurrent));
}
