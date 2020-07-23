/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <actor_system.h>

#include <caf/all.hpp>

const uint16_t yanghui_job_port1 = 55011;
const uint16_t yanghui_job_port2 = 55012;
const uint16_t yanghui_job_port3 = 55013;
const uint16_t yanghui_job_port4 = 55014;

std::vector<std::vector<int>> kYanghuiData2 = {
    {5},
    {7, 8},
    {2, 1, 4},
    {4, 2, 6, 1},
    {2, 7, 3, 4, 5},
    {2, 3, 7, 6, 8, 3},
    {2, 3, 4, 4, 2, 7, 7},
    {8, 4, 4, 3, 4, 5, 6, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7},
    {1, 2, 8, 5, 6, 2, 9, 1, 1, 8, 9, 9, 1, 6, 8, 7, 7, 6}};

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;
  int worker_load = 0;

  config() {
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(worker_load, "load, l", "load balance worker sleep second")
        .add(node_keeper_port, "node_port", "set node keeper port");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

void caf_main(caf::actor_system& system, const config& cfg) {
  auto yanghui_job_actor1 =
      system.middleman().remote_actor(cfg.root_host, yanghui_job_port1);
  if (!yanghui_job_actor1)
    std::cerr << "unable to connect to yanghui_job_actor1: "
              << to_string(yanghui_job_actor1.error()) << '\n';

  auto yanghui_job_actor2 =
      system.middleman().remote_actor(cfg.root_host, yanghui_job_port2);
  if (!yanghui_job_actor2)
    std::cerr << "unable to connect to yanghui_job_actor2: "
              << to_string(yanghui_job_actor2.error()) << '\n';

  auto yanghui_job_actor3 =
      system.middleman().remote_actor(cfg.root_host, yanghui_job_port3);
  if (!yanghui_job_actor3)
    std::cerr << "unable to connect to yanghui_job_actor3: "
              << to_string(yanghui_job_actor3.error()) << '\n';

  auto yanghui_job_actor4 =
      system.middleman().remote_actor(cfg.root_host, yanghui_job_port4);
  if (!yanghui_job_actor4)
    std::cerr << "unable to connect to yanghui_job_actor4: "
              << to_string(yanghui_job_actor4.error()) << '\n';

  caf::scoped_actor self{system};

  std::vector<caf::actor> yanghui_jobs;
  yanghui_jobs.push_back(*yanghui_job_actor1);
  yanghui_jobs.push_back(*yanghui_job_actor2);
  yanghui_jobs.push_back(*yanghui_job_actor3);
  yanghui_jobs.push_back(*yanghui_job_actor4);

  while (true) {
    for (const auto& yanghui_job : yanghui_jobs) {
      self->request(yanghui_job, std::chrono::seconds(1), kYanghuiData2)
          .receive(
              [&](bool status, int result) {
                aout(self) << "status: " << status << " -> " << result
                           << std::endl;
              },
              [&](caf::error& err) { aout(self) << "cell #" << std::endl; });
      ;
    }
  }
}

CAF_MAIN(caf::io::middleman)