//
// Created by Mingfei Deng on 2020/7/6.
//

#ifndef CDCF_YANGHUI_ACTOR_H
#define CDCF_YANGHUI_ACTOR_H

#include <actor_system.h>
#include "./CounterInterface.h"

caf::behavior yanghui(caf::event_based_actor* self, CounterInterface* counter);

#endif  // CDCF_YANGHUI_ACTOR_H
