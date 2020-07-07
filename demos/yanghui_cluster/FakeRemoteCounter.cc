//
// Created by Mingfei Deng on 2020/7/7.
//

#include "FakeRemoteCounter.h"

#include <future>

#include "yanghui_config.h"

int FakeRemoteCounter::AddNumber(int a, int b, int& result) {
  int ret = 0;
  std::promise<int> promise;

  send_actor_->request(count_actor_, caf::infinite, a, b)
      .receive(
          [&](int remove_result) {
            result = remove_result;
            promise.set_value(0);
          },
          [&](caf::error& err) { promise.set_value(1); });

  ret = promise.get_future().get();

  return ret;
}

int FakeRemoteCounter::Compare(std::vector<int> numbers, int& min) {
  int ret = 0;
  std::promise<int> promise;

  NumberCompareData send_data;
  send_data.numbers = numbers;

  send_actor_->request(count_actor_, caf::infinite, send_data)
      .receive(
          [&](int remove_result) {
            min = remove_result;
            promise.set_value(0);
          },
          [&](caf::error& err) { promise.set_value(1); });

  ret = promise.get_future().get();

  return ret;
}
