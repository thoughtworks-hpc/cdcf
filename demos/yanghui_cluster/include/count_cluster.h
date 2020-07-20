/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_COUNT_CLUSTER_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_COUNT_CLUSTER_H_
#include <actor_system.h>

#include <string>
#include <vector>

#include "../counter_interface.h"
#include "../yanghui_config.h"
const uint16_t k_yanghui_work_port1 = 55001;
const uint16_t k_yanghui_work_port2 = 55002;
const uint16_t k_yanghui_work_port3 = 55003;
const uint16_t k_yanghui_work_port4 = 55004;

class CountCluster : public actor_system::cluster::Observer,
                     public counter_interface {
 public:
  virtual void AddWorkerNode(const std::string& host) = 0;
  //  virtual int AddNumber(int a, int b, int& result) = 0;
  //  virtual int Compare(std::vector<int> numbers, int& min) = 0;
  std::string host_;

  explicit CountCluster(std::string host);
  virtual ~CountCluster();
  void InitWorkerNodes();
  void Update(const actor_system::cluster::Event& event) override;
};

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_COUNT_CLUSTER_H_
