//
// Created by Mingfei Deng on 2020/2/17.
//
#include "comm_example.h"
int subFun(int a, int b) { return a + b; }

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](add_atom, int a, int b) -> int {
            aout(self) << "received task from a remote node" << endl;
            return a + b;
          },
          [=](sub_atom, int a, int b) -> int {
            aout(self) << "received task from a remote node" << endl;
            return subFun(a, b);
          }};
}