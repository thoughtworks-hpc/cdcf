/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_SIMPLE_ACTOR_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_SIMPLE_ACTOR_H_

#include <actor_system.h>

#include <map>
#include <vector>

struct YanghuiState {
  int index = 0;
  std::map<int, int> current_result;
  std::vector<std::vector<int> > data;
};

struct GetMinState {
  int count = 0;
  std::map<int, int> current_result;
};

caf::behavior yanghui_get_final_result(caf::stateful_actor<GetMinState>* self,
                                       const caf::actor& worker,
                                       const caf::actor& out_data);

caf::behavior yanghui_count_path(caf::stateful_actor<YanghuiState>* self,
                                 const caf::actor& worker,
                                 const caf::actor& out_data);

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_SIMPLE_ACTOR_H_
