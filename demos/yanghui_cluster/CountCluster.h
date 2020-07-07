//
// Created by Mingfei Deng on 2020/7/6.
//

#ifndef CDCF_COUNTCLUSTER_H
#define CDCF_COUNTCLUSTER_H
#include <actor_system.h>

#include "./CounterInterface.h"
#include "./yanghui_config.h"

const uint16_t k_yanghui_work_port1 = 55001;
const uint16_t k_yanghui_work_port2 = 55002;
const uint16_t k_yanghui_work_port3 = 55003;

class CountCluster : public actor_system::cluster::Observer,
                     public CounterInterface {
 public:
  virtual void AddWorkerNode(const std::string& host) = 0;
  //  virtual int AddNumber(int a, int b, int& result) = 0;
  //  virtual int Compare(std::vector<int> numbers, int& min) = 0;
  std::string host_;

  explicit CountCluster(std::string host);
  virtual ~CountCluster();
  void InitWorkerNodes(
      const std::vector<actor_system::cluster::Member>& members,
      const std::string& host);
  void Update(const actor_system::cluster::Event& event) override;
};

#endif  // CDCF_COUNTCLUSTER_H
