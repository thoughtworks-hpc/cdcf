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

void RouterPoolCountCluster::Update(const actor_system::cluster::Event& event) {
  if (event.member.hostname != host_) {
    if (event.member.status == event.member.Down) {
      RemoveNode(event.member.host, worker_port_);
      return;
    }
  }
  CountCluster::Update(event);
}

void RouterPoolCountCluster::AddWorkerNode(const std::string& host) {
  if (!pool_) {
    return;
  }
  caf::scoped_actor self(system_);
  self->send(pool_, caf::sys_atom::value, caf::add_atom::value,
             cdcf::router_pool::node_atom::value, host, worker_port_);
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

std::string RouterPoolCountCluster::NodeList() {
  std::promise<std::string> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::node_atom::value)
      .receive(
          [&](std::vector<std::string>& ret) {
            std::string info;
            for (auto& it : ret) {
              info += " ";
              info += it;
            }
            result.set_value(info);
          },
          [&](const caf::error& err) { result.set_value("ERROR"); });
  return result.get_future().get();
}

size_t RouterPoolCountCluster::GetPoolSize() {
  std::promise<size_t> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value)
      .receive(
          [&](std::vector<caf::actor>& ret) { result.set_value(ret.size()); },
          [&](const caf::error& err) { result.set_value(0); });
  return result.get_future().get();
}

size_t RouterPoolCountCluster::GetPoolSize(const std::string& host,
                                           uint16_t port) {
  std::promise<size_t> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::get_atom::value, cdcf::router_pool::actor_atom::value,
                host, port)
      .receive(
          [&](std::vector<caf::actor>& ret) { result.set_value(ret.size()); },
          [&](const caf::error& err) { result.set_value(0); });
  return result.get_future().get();
}

bool RouterPoolCountCluster::ChangePoolSize(size_t size) {
  std::promise<bool> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::update_atom ::value, size)
      .receive([&](bool& ret) { result.set_value(ret); },
               [&](const caf::error& err) { result.set_value(false); });
  return result.get_future().get();
}

bool RouterPoolCountCluster::ChangePoolSize(size_t size,
                                            const std::string& host,
                                            uint16_t port) {
  std::promise<bool> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::update_atom::value, size, host, port)
      .receive([&](bool& ret) { result.set_value(ret); },
               [&](const caf::error& err) { result.set_value(false); });
  return result.get_future().get();
}

bool RouterPoolCountCluster::RemoveNode(const std::string& host,
                                        uint16_t port) {
  std::promise<bool> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::delete_atom::value, cdcf::router_pool::node_atom::value,
                host, port)
      .receive([&](bool& ret) { result.set_value(ret); },
               [&](const caf::error& err) { result.set_value(false); });
  return result.get_future().get();
}

bool RouterPoolCountCluster::AddNode(const std::string& host, uint16_t port) {
  std::promise<bool> result;
  caf::scoped_actor self(system_);
  self->request(pool_, caf::infinite, caf::sys_atom::value,
                caf::add_atom::value, cdcf::router_pool::node_atom::value, host,
                port)
      .receive([&](bool& ret) { result.set_value(ret); },
               [&](const caf::error& err) { result.set_value(false); });
  return result.get_future().get();
}
