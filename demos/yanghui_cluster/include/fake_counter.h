/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_COUNTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_COUNTER_H_
#include <vector>

#include "../counter_interface.h"

class fake_counter : public counter_interface {
 public:
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_FAKE_COUNTER_H_
