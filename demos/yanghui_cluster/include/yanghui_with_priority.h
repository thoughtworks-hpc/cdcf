/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_WITH_PRIORITY_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_WITH_PRIORITY_H_

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "./yanghui_io.h"
#include "cdcf/cluster/cluster.h"
#include "cdcf/logger.h"

class WorkerPool : public cdcf::cluster::Observer {
 public:
  WorkerPool(caf::actor_system& system, std::string host, uint16_t worker_port,
             YanghuiIO& yanghui_io)
      : system_(system),
        host_(std::move(host)),
        worker_port_(worker_port),
        yanghui_io_(yanghui_io) {
    auto context = new caf::scoped_execution_unit(&system_);
    this->pool_ =
        caf::actor_pool::make(context, caf::actor_pool::round_robin());
  }

  int Init();

  bool IsEmpty() const;

  caf::actor& GetPool();

 private:
  int AddWorker(const std::string& host);

  void Update(const cdcf::cluster::Event& event) override;

  void PrintClusterMembers();

  caf::actor pool_;
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
