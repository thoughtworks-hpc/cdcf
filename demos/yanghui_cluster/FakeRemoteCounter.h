//
// Created by Mingfei Deng on 2020/7/7.
//

#ifndef CDCF_FAKEREMOTECOUNTER_H
#define CDCF_FAKEREMOTECOUNTER_H
#include <utility>
#include <vector>

#include "CounterInterface.h"
#include "caf/all.hpp"
#include "yanghui_config.h"

static caf::actor_system_config* cfg_yanghui;
static caf::actor_system* sys_yanghui;

class FakeRemoteCounter : public CounterInterface {
 public:
  FakeRemoteCounter(
      decltype((*sys_yanghui).spawn<typed_calculator>()) countActor,
      caf::scoped_actor& sendActor)
      : count_actor_(std::move(countActor)), send_actor_(sendActor) {}

  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

 private:
  caf::scoped_actor& send_actor_;
  decltype((*sys_yanghui).spawn<typed_calculator>()) count_actor_;
};

#endif  // CDCF_FAKEREMOTECOUNTER_H
