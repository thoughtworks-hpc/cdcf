/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "include/actor_union_count_cluster.h"

#include <limits.h>

#include <caf/openssl/all.hpp>

void ActorUnionCountCluster::AddWorkerNodeWithPort(const std::string& host,
                                                   uint16_t port) {
  CDCF_LOGGER_DEBUG("try to connect to remote actor, host: {}, port: {}", host,
                    port);
  auto worker_actor = yanghui_io_.remote_actor(system_, host, port);
  if (!worker_actor) {
    CDCF_LOGGER_ERROR("connect remote actor failed. host:{}, port:{}, error{}",
                      host, port, system_.render(worker_actor.error()));
    return;
  }

  counter_.AddActor(*worker_actor);

  CDCF_LOGGER_INFO("add pool member host:{}, port:{}", host, port);
}

void ActorUnionCountCluster::AddWorkerNode(const std::string& host) {
  AddWorkerNodeWithPort(host, k_yanghui_work_port1);
  AddWorkerNodeWithPort(host, k_yanghui_work_port2);
  AddWorkerNodeWithPort(host, k_yanghui_work_port3);

  auto worker_actor =
      yanghui_io_.remote_actor(system_, host, k_yanghui_work_port4);
  if (!worker_actor) {
    CDCF_LOGGER_ERROR("connect remote actor failed. host:{}, port:{}", host, k_yanghui_work_port4);
    return;
  }

  caf::anon_send(load_balance_, caf::sys_atom::value, caf::put_atom::value,
                 *worker_actor);
}

int ActorUnionCountCluster::AddNumber(int a, int b, int& result) {
  int error = 0;
  std::promise<int> promise;
  CDCF_LOGGER_INFO("start add task input:{}, {}", a, b);

  counter_.SendAndReceive([&](int ret) { promise.set_value(ret); },
                          [&](const caf::error& err) {
                            promise.set_value(INT_MAX);
                            error = 1;
                            CDCF_LOGGER_ERROR("error: {}", caf::to_string(err));
                          },
                          a, b);

  result = promise.get_future().get();
  CDCF_LOGGER_INFO("get result::{}.", result);
  return error;
}

int ActorUnionCountCluster::Compare(std::vector<int> numbers, int& min) {
  int error = 0;
  std::promise<int> promise;

  std::string input_data_str;
  for (int p : numbers) {
    input_data_str = input_data_str + " " + std::to_string(p);
  }

  CDCF_LOGGER_INFO("start compare task. input data:{}", input_data_str);

  NumberCompareData send_data;
  send_data.numbers = numbers;

  counter_.SendAndReceive(
      [&](int ret) {
        min = ret;
        promise.set_value(ret);
      },
      [&](const caf::error& err) {
        promise.set_value(INT_MAX);
        error = 1;
        CDCF_LOGGER_ERROR("error: {}", caf::to_string(err));
      },
      send_data);

  min = promise.get_future().get();
  CDCF_LOGGER_INFO("get min result:{}", min);

  return error;
}
