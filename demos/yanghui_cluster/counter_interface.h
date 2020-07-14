/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_COUNTER_INTERFACE_H_
#define DEMOS_YANGHUI_CLUSTER_COUNTER_INTERFACE_H_
#include <vector>

class counter_interface {
 public:
  virtual int AddNumber(int a, int b, int& result) = 0;
  virtual int Compare(std::vector<int> numbers, int& min) = 0;
};

#endif  // DEMOS_YANGHUI_CLUSTER_COUNTER_INTERFACE_H_
