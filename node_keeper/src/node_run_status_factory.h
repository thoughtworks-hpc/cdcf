//
// Created by Yuecheng Pei on 2020/8/5.
//

#ifndef CDCF_NODE_RUN_STATUS_FACTORY_H
#define CDCF_NODE_RUN_STATUS_FACTORY_H
#include "node_run_status.h"

class NodeRunStatusFactory {
 public:
  NodeRunStatus *GetInstance() {
    return NodeRunStatus::GetInstance();
  };
};

#endif  // CDCF_NODE_RUN_STATUS_FACTORY_H
