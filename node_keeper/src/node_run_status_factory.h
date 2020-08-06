/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */

#ifndef NODE_KEEPER_SRC_NODE_RUN_STATUS_FACTORY_H_
#define NODE_KEEPER_SRC_NODE_RUN_STATUS_FACTORY_H_
#include "./node_run_status.h"

class NodeRunStatusFactory {
 public:
  virtual NodeRunStatus *GetInstance() { return NodeRunStatus::GetInstance(); }
};

#endif  // NODE_KEEPER_SRC_NODE_RUN_STATUS_FACTORY_H_
