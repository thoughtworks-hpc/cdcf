/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#ifndef NODE_KEEPER_SRC_CHANNEL_H_
#define NODE_KEEPER_SRC_CHANNEL_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace node_keeper {
template <class Item>
class Channel {
 public:
  Channel() : closed_(false) {}

  void Close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    condition_variable_.notify_all();
  }

  bool IsClosed() {
    std::unique_lock<std::mutex> lock(mutex_);
    return closed_;
  }

  void Put(const Item &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (closed_) {
      throw std::logic_error("put to closed channel");
    }
    queue_.push(item);
    condition_variable_.notify_one();
  }

  std::pair<bool, Item> Get(bool wait = true) {
    std::pair<bool, Item> result;
    std::unique_lock<std::mutex> lock(mutex_);
    if (wait) {
      condition_variable_.wait(lock,
                               [&]() { return closed_ || !queue_.empty(); });
    }
    if (queue_.empty()) {
      return result;
    }
    result.first = true;
    result.second = queue_.front();
    queue_.pop();
    return result;
  }

 private:
  std::queue<Item> queue_;
  std::mutex mutex_;
  std::condition_variable condition_variable_;
  bool closed_;
};
}  // namespace node_keeper
#endif  // NODE_KEEPER_SRC_CHANNEL_H_
