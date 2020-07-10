/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_REMOTE_COUNTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_REMOTE_COUNTER_H_
#include <utility>
#include <vector>

#include "../counter_interface.h"
#include "../yanghui_config.h"
#include "caf/all.hpp"

static caf::actor_system_config* cfg_yanghui;
static caf::actor_system* sys_yanghui;

class fake_remote_counter : public counter_interface {
 public:
  fake_remote_counter(
      decltype((*sys_yanghui).spawn<typed_calculator>()) countActor,
      caf::scoped_actor& sendActor)
      : count_actor_(std::move(countActor)), send_actor_(sendActor) {}

  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;

 private:
  caf::scoped_actor& send_actor_;
  decltype((*sys_yanghui).spawn<typed_calculator>()) count_actor_;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_REMOTE_COUNTER_H_
