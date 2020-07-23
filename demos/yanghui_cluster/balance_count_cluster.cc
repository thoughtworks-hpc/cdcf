/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/balance_count_cluster.h"

#include <string>
#include <utility>
#include <vector>

#include <caf/openssl/all.hpp>
BalanceCountCluster::BalanceCountCluster(std::string host,
                                         caf::actor_system& system)
    : CountCluster(host),
      system_(system),
      context_(&system_),
      host_(std::move(host)),
      sender_actor_(system_) {
  auto policy = cdcf::load_balancer::policy::MinLoad(1);
  counter_ = cdcf::load_balancer::Router::Make(&context_, std::move(policy));
}

void BalanceCountCluster::AddWorkerNodeWithPort(const std::string& host,
                                                uint16_t port) {
  auto worker_actor = caf::openssl::remote_actor(system_, host, port);
  if (!worker_actor) {
    std::cout << "connect remote actor failed. host:" << host
              << ", port:" << port << std::endl;
  }

  caf::anon_send(counter_, caf::sys_atom::value, caf::put_atom::value,
                 *worker_actor);

  std::cout << "=======add pool member host:" << host << ", port:" << port
            << std::endl;
}
void BalanceCountCluster::AddWorkerNode(const std::string& host) {
  AddWorkerNodeWithPort(host, k_yanghui_work_port1);
  AddWorkerNodeWithPort(host, k_yanghui_work_port2);
  AddWorkerNodeWithPort(host, k_yanghui_work_port3);
}

int BalanceCountCluster::AddNumber(int a, int b, int& result) {
  int error = 0;
  std::promise<int> promise;

  std::cout << "start add task input:" << a << ", " << b << std::endl;

  sender_actor_->request(counter_, std::chrono::seconds(5), a, b)
      .receive([&](int ret) { promise.set_value(ret); },
               [&](const caf::error& err) {
                 std::cout << "send add request get err: "
                           << caf::to_string(err) << "input data: a=" << a
                           << ", b=" << b << std::endl;
                 error = 1;
               });

  result = promise.get_future().get();

  std::cout << "get result:" << result << std::endl;
  return error;
}
int BalanceCountCluster::Compare(std::vector<int> numbers, int& min) {
  int error = 0;
  std::promise<int> promise;
  std::cout << "start compare task. input data:" << std::endl;

  for (int p : numbers) {
    std::cout << p << " ";
  }

  std::cout << std::endl;

  NumberCompareData send_data;
  send_data.numbers = numbers;

  sender_actor_->request(counter_, std::chrono::seconds(5), send_data)
      .receive(
          [&](int ret) {
            min = ret;
            promise.set_value(ret);
          },
          [&](const caf::error& err) {
            std::cout << "send add request get err: " << caf::to_string(err)
                      << "input data:";
            for (auto num : numbers) {
              std::cout << num << ", ";
            }
            std::cout << std::endl;
            error = 1;
          });

  min = promise.get_future().get();
  std::cout << "get min:" << min << std::endl;

  return error;
}
