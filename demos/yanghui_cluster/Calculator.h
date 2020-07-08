//
// Created by Yuecheng Pei on 2020/7/8.
//
#ifndef CDCF_CALCULATOR_H
#define CDCF_CALCULATOR_H
#include <condition_variable>

#include <caf/all.hpp>

#include "./BalanceCountCluster.h"


calculator::behavior_type calculator_fun(calculator::pointer self);
calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro);
#endif  // CDCF_CALCULATOR_H
