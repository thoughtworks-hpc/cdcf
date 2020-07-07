//
// Created by Mingfei Deng on 2020/7/6.
//

#ifndef CDCF_COUNTERINTERFACE_H
#define CDCF_COUNTERINTERFACE_H

class CounterInterface {
 public:
  virtual int AddNumber(int a, int b, int& result) = 0;
  virtual int Compare(std::vector<int> numbers, int& min) = 0;
};

#endif  // CDCF_COUNTERINTERFACE_H
