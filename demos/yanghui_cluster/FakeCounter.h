//
// Created by Mingfei Deng on 2020/7/6.
//

#ifndef CDCF_FAKECOUNTER_H
#define CDCF_FAKECOUNTER_H
#include <vector>

#include "CounterInterface.h"

class FakeCounter : public CounterInterface{
 public:
  int AddNumber(int a, int b, int& result) override;
  int Compare(std::vector<int> numbers, int& min) override;
};

#endif  // CDCF_FAKECOUNTER_H
