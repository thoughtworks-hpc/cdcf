/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#include "src/queue.h"

namespace queue {

TimedFunctorQueue::TimedFunctorQueue(std::chrono::milliseconds interval)
    : interval_(interval), should_stop_(false) {
  thread_ = std::make_unique<std::thread>([this]() { CallFunctors(); });
}

TimedFunctorQueue::~TimedFunctorQueue() {
  {
    std::lock_guard lk(should_stop_mut_);
    should_stop_ = true;
  }

  thread_->join();
}

void TimedFunctorQueue::Push(const std::function<void()>& functor, int times) {
  if (times < 1) {
    return;
  }

  std::lock_guard<std::mutex> lk(queue_mut_);
  queue_.push(std::make_pair(functor, times));
  queue_cond_.notify_one();
}
void TimedFunctorQueue::CallFunctors() {
  while (true) {
    {
      std::lock_guard lk(should_stop_mut_);
      if (should_stop_) {
        break;
      }
    }

    if (!queue_.empty()) {
      std::unique_lock<std::mutex> lk(queue_mut_);
      queue_cond_.wait(lk, [this]() { return !queue_.empty(); });
      std::pair pair = queue_.front();
      queue_.pop();
      lk.unlock();

      for (int i = 0; i < pair.second; ++i) {
        pair.first();
        std::this_thread::sleep_for(interval_);
      }
    }
  }
}

};  // namespace queue
