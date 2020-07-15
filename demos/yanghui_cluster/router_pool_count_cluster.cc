/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "include/router_pool_count_cluster.h"

RouterPoolCountCluster::RouterPoolCountCluster(
    std::string host, caf::actor_system& system, uint16_t port,
    uint16_t worker_port, std::string& routee_name, caf::message& routee_args,
    caf::actor_system::mpi& routee_ifs, size_t& default_actor_num,
    caf::actor_pool::policy& policy)
    : CountCluster(host),
      host_(std::move(host)),
      system_(system),
      port_(port),
      worker_port_(worker_port) {
  std::string pool_name = "pool name";
  std::string pool_description = "pool description";
  pool_ = system.spawn<cdcf::router_pool::RouterPool>(
      pool_name, pool_description, routee_name, routee_args, routee_ifs,
      default_actor_num, policy);
}
void RouterPoolCountCluster::AddWorkerNode(const std::string& host) {
  if (!pool_) {
    return;
  }
  caf::scoped_actor self(system_);
  self->send(pool_, caf::sys_atom::value, caf::add_atom::value, host,
             worker_port_);
}

int RouterPoolCountCluster::AddNumber(int a, int b, int& result) {
  int error = 0;
  std::promise<int> promise;

  std::cout << "start add task input:" << a << ", " << b << std::endl;

  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, a, b)
      .receive([&](int ret) { promise.set_value(ret); },
               [&](const caf::error& err) { error = 1; });

  result = promise.get_future().get();

  std::cout << "get result:" << result << std::endl;
  return error;
}

int RouterPoolCountCluster::Compare(std::vector<int> numbers, int& min) {
  int error = 0;
  std::promise<int> promise;
  std::cout << "start compare task. input data:" << std::endl;

  for (int p : numbers) {
    std::cout << p << " ";
  }

  std::cout << std::endl;
  caf::scoped_actor self(system_);

  NumberCompareData send_data;
  send_data.numbers = numbers;

  self->request(pool_, caf::infinite, send_data)
      .receive(
          [&](int ret) {
            min = ret;
            promise.set_value(ret);
          },
          [&](const caf::error& err) { error = 1; });

  min = promise.get_future().get();
  std::cout << "get min:" << min << std::endl;

  return error;
}
