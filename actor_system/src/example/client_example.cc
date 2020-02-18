//
// Created by Mingfei Deng on 2020/2/17.
//
#include "actor_system.h"
#include "comm_example.h"

void clientRun(function_view<calculator> actor_function) {
  int a = 30;
  int b = 34;
  auto c = actor_function(add_atom::value, a, b);
  int d = c.value();
  cout << a << "+" << b << "=" << d << endl;
  getchar();
}

ACTOR_CLIENT_MAIN(clientRun)
