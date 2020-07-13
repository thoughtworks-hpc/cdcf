//
// Created by Mingfei Deng on 2020/7/11.
//

#ifndef CDCF_SIMPLE_COUNTER_H
#define CDCF_SIMPLE_COUNTER_H

#include "./yanghui_config.h"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

caf::behavior simple_counter(caf::event_based_actor* self);

#endif  // CDCF_SIMPLE_COUNTER_H
