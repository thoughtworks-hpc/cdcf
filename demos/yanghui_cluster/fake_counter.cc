/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/fake_counter.h"

#include <limits.h>

int fake_counter::AddNumber(int a, int b, int& result) {
  result = a + b;
  return 0;
}

int fake_counter::Compare(std::vector<int> numbers, int& min) {
  min = INT_MAX;
  for (auto one_number : numbers) {
    if (min > one_number) {
      min = one_number;
    }
  }

  return 0;
}
