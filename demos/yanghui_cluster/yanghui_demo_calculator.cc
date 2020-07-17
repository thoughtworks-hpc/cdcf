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
      [=](int a, int b, int position) -> std::string { return "empty"; },
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
          // currently, for remotely spawned actor, it seems caf does not
          // support return types other than c++ primitive types and std::string
          [=](int a, int b, int position) -> std::string {
            std::cout << "received add task. input a:" << a << " b:" << b
                      << std::endl;
            std::string result;
            result =
                result + std::to_string(a + b) + ":" + std::to_string(position);

            std::cout << "return: " << result << std::endl;
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

bool operator==(const NumberCompareData& lhs, const NumberCompareData& rhs) {
  return lhs.numbers == rhs.numbers && lhs.index == rhs.index;
}
