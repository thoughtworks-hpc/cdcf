/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef CDCF_NODE_RUN_STATUS_FACTORY_H
#define CDCF_NODE_RUN_STATUS_FACTORY_H
#include "node_run_status.h"

class NodeRunStatusFactory {
 public:
  virtual NodeRunStatus *GetInstance() {
    return NodeRunStatus::GetInstance();
  };
};

#endif  // CDCF_NODE_RUN_STATUS_FACTORY_H
