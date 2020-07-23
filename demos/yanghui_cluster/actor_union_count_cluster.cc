/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "include/actor_union_count_cluster.h"

#include <caf/openssl/all.hpp>

void ActorUnionCountCluster::AddWorkerNodeWithPort(const std::string& host,
                                                   uint16_t port) {
  auto worker_actor = caf::openssl::remote_actor(system_, host, port);
  if (!worker_actor) {
    std::cout << "connect remote actor failed. host:" << host
              << ", port:" << port
              << ", error:" << system_.render(worker_actor.error())
              << std::endl;
    return;
  }

  counter_.AddActor(*worker_actor);

  std::cout << "=======add pool member host:" << host << ", port:" << port
            << std::endl;
}
void ActorUnionCountCluster::AddWorkerNode(const std::string& host) {
  AddWorkerNodeWithPort(host, k_yanghui_work_port1);
  AddWorkerNodeWithPort(host, k_yanghui_work_port2);
  AddWorkerNodeWithPort(host, k_yanghui_work_port3);

  auto worker_actor =
      system_.middleman().remote_actor(host, k_yanghui_work_port4);
  if (!worker_actor) {
    std::cout << "connect remote actor failed. host:" << host
              << ", port:" << k_yanghui_work_port4 << std::endl;
  }

  caf::anon_send(load_balance_, caf::sys_atom::value, caf::put_atom::value,
                 *worker_actor);
}

int ActorUnionCountCluster::AddNumber(int a, int b, int& result) {
  int error = 0;
  std::promise<int> promise;

  std::cout << "start add task input:" << a << ", " << b << std::endl;

  counter_.SendAndReceive([&](int ret) { promise.set_value(ret); },
                          [&](const caf::error& err) { error = 1; }, a, b);

  result = promise.get_future().get();

  std::cout << "get result:" << result << std::endl;
  return error;
}

int ActorUnionCountCluster::Compare(std::vector<int> numbers, int& min) {
  int error = 0;
  std::promise<int> promise;
  std::cout << "start compare task. input data:" << std::endl;

  for (int p : numbers) {
    std::cout << p << " ";
  }

  std::cout << std::endl;

  NumberCompareData send_data;
  send_data.numbers = numbers;

  counter_.SendAndReceive(
      [&](int ret) {
        min = ret;
        promise.set_value(ret);
      },
      [&](const caf::error& err) { error = 1; }, send_data);

  min = promise.get_future().get();
  std::cout << "get min:" << min << std::endl;

  return error;
}
