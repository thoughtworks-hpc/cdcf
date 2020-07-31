/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_WITH_PRIORITY_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_WITH_PRIORITY_H_

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../logger/include/logger.h"
#include "../yanghui_config.h"
#include "./yanghui_io.h"

class WorkerPool : public actor_system::cluster::Observer {
 public:
  WorkerPool(caf::actor_system& system, std::string host, uint16_t worker_port,
             YanghuiIO& yanghui_io)
      : system_(system),
        host_(std::move(host)),
        worker_port_(worker_port),
        yanghui_io_(yanghui_io) {}

  int Init();

  bool IsEmpty();

  caf::strong_actor_ptr GetWorker();

 private:
  int AddWorker(const std::string& host);

  void Update(const actor_system::cluster::Event& event) override;

  void PrintClusterMembers();

  mutable std::shared_mutex workers_mutex_;
  std::vector<caf::strong_actor_ptr> workers_;
  std::atomic<int> worker_index_ = 0;
  std::string host_;
  caf::actor_system& system_;
  uint16_t worker_port_;
  YanghuiIO& yanghui_io_;
};

struct yanghui_state {
  int level_ = 1;
  int count_ = 0;
  std::vector<std::vector<int>> triangle_data_;
  std::vector<int> last_level_results_;
  caf::strong_actor_ptr triangle_sender_;
};

using start_atom = caf::atom_constant<caf::atom("start")>;
using end_atom = caf::atom_constant<caf::atom("end")>;

caf::behavior yanghui_with_priority(caf::stateful_actor<yanghui_state>* self,
                                    WorkerPool* worker_pool,
                                    bool is_high_priority = false);

struct dispatcher_state {
  std::vector<std::pair<bool, int>> result;
  caf::strong_actor_ptr triangle_sender_;
};

caf::behavior yanghui_job_dispatcher(
    caf::stateful_actor<dispatcher_state>* self, caf::actor target1,
    caf::actor target2);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_WITH_PRIORITY_H_
