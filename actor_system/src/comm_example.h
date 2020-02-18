//
// Created by Mingfei Deng on 2020/2/17.
//

#ifndef CDCF_COMM_EXAMPLE_H
#define CDCF_COMM_EXAMPLE_H
#include "actor_system.h"

using add_atom = atom_constant<Operation("add")>;
using sub_atom = atom_constant<Operation("sub")>;

using calculator = typed_actor<replies_to<add_atom, int, int>::with<int>,
                               replies_to<sub_atom, int, int>::with<int>>;

calculator::behavior_type calculator_fun(calculator::pointer self);
SYSTEM_CONFIG("calculator", calculator_fun)
#endif  // CDCF_COMM_EXAMPLE_H
