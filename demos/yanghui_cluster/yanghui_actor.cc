/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "include/yanghui_actor.h"

#include <limits.h>

#include <algorithm>
#include <string>
#include <vector>
caf::behavior yanghui(caf::event_based_actor* self,
                      counter_interface* counter) {
  return {
      [=](const std::vector<std::vector<int>>& data) {
        int n = data.size();
        //        int temp_states[n];
        //        int states[n];
        int* temp_states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
        int* states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
        int error = 0;

        states[0] = 1;
        states[0] = data[0][0];
        int i, j, k, min_sum = INT_MAX;
        for (i = 1; i < n; i++) {
          for (j = 0; j < i + 1; j++) {
            if (j == 0) {
              // temp_states[0] = states[0] + data[i][j];
              error = counter->AddNumber(states[0], data[i][j], temp_states[0]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            } else if (j == i) {
              // temp_states[j] = states[j - 1] + data[i][j];
              error =
                  counter->AddNumber(states[j - 1], data[i][j], temp_states[j]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            } else {
              // temp_states[j] = std::min(states[j - 1], states[j]) +
              // data[i][j];
              error = counter->AddNumber(std::min(states[j - 1], states[j]),
                                         data[i][j], temp_states[j]);
              if (0 != error) {
                caf::aout(self) << "cluster down, exit task" << std::endl;
                return INT_MAX;
              }
            }
          }

          for (k = 0; k < i + 1; k++) {
            states[k] = temp_states[k];
          }
        }

        //    for (j = 0; j < n; j++) {
        //      if (states[j] < min_sum) min_sum = states[j];
        //    }

        std::vector<int> states_vec(states, states + n);

        error = counter->Compare(states_vec, min_sum);
        if (0 != error) {
          caf::aout(self) << "cluster down, exit task" << std::endl;
          return INT_MAX;
        }

        caf::aout(self) << "yanghui triangle actor task complete, result: "
                        << min_sum << std::endl;
        free(temp_states);
        free(states);
        return min_sum;
      },
      [=](std::string&) {
        caf::aout(self) << "simulate get a critical error， yanghui actor quit."
                        << std::endl;
        self->quit();
        return 0;
      }};
}
