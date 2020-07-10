/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef ROUTER_POOL_INCLUDE_ROUTER_POOL_MESSAGE_H_
#define ROUTER_POOL_INCLUDE_ROUTER_POOL_MESSAGE_H_

#include <actor_system.h>

#include <string>
#include <vector>

namespace cdcf::router_pool {

struct RouterPoolMsgData {
  size_t min_num;
  size_t max_num;
  std::vector<caf::actor> actors;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const RouterPoolMsgData& x) {
  return f(caf::meta::type_name("RouterPoolMsgData"), x.min_num, x.max_num,
           x.actors);
}

}  // namespace cdcf::router_pool

#endif  // ROUTER_POOL_INCLUDE_ROUTER_POOL_MESSAGE_H_
