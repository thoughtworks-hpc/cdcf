/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "include/yanghui_demo_calculator.h"

calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro) {
  return {
      [self, &deal_msg_count, sleep_micro](int a, int b) -> int {
        ++deal_msg_count;
        caf::aout(self) << "slow calculator received add task. input a:" << a
                        << " b:" << b
                        << ", ************* calculator sleep microseconds:"
                        << sleep_micro << " msg count:" << deal_msg_count
                        << std::endl;
        if (sleep_micro) {
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_micro));
        }

        int result = a + b;
        caf::aout(self) << "return: " << result << std::endl;
        return result;
      },
      [self, &deal_msg_count, sleep_micro](NumberCompareData& data) -> int {
        ++deal_msg_count;
        if (data.numbers.empty()) {
          caf::aout(self) << "get empty compare" << std::endl;
          return 999;
        }

        int result = data.numbers[0];

        caf::aout(self) << "received compare task, input: ";

        for (int number : data.numbers) {
          caf::aout(self) << number << " ";
          if (number < result) {
            result = number;
          }
        }

        caf::aout(self) << "************* calculator sleep microseconds:"
                        << sleep_micro << " msg count:" << deal_msg_count
                        << std::endl;

        if (sleep_micro) {
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_micro));
        }

        caf::aout(self) << "return: " << result << std::endl;

        return result;
      }};
}

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](int a, int b) -> int {
            caf::aout(self) << "received add task. input a:" << a << " b:" << b
                            << std::endl;

            int result = a + b;
            caf::aout(self) << "return: " << result << std::endl;
            return result;
          },
          [=](NumberCompareData& data) -> int {
            if (data.numbers.empty()) {
              caf::aout(self) << "get empty compare" << std::endl;
              return 999;
            }

            int result = data.numbers[0];

            caf::aout(self) << "received compare task, input: ";

            for (int number : data.numbers) {
              caf::aout(self) << number << " ";
              if (number < result) {
                result = number;
              }
            }

            caf::aout(self) << std::endl;
            caf::aout(self) << "return: " << result << std::endl;

            return result;
          }};
}
