/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_QUEUE_H_
#define NODE_KEEPER_SRC_QUEUE_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace queue {

class TimedFunctorQueue {
 public:
  explicit TimedFunctorQueue(std::chrono::milliseconds interval);
  ~TimedFunctorQueue();
  TimedFunctorQueue(const TimedFunctorQueue&) = delete;
  TimedFunctorQueue& operator=(const TimedFunctorQueue&) = delete;
  void Push(const std::function<void()>& functor, int times);

 private:
  void CallFunctors();

  std::chrono::milliseconds interval_;
  std::queue<std::pair<std::function<void()>, int> > queue_;
  mutable std::mutex queue_mut_;
  std::condition_variable queue_cond_;
  std::unique_ptr<std::thread> thread_;
  bool should_stop_;
  mutable std::mutex should_stop_mut_;
};

};      // namespace queue
#endif  // NODE_KEEPER_SRC_QUEUE_H_
