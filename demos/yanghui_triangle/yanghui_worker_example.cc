/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./yanghui_config.h"

caf::behavior countAdd(caf::event_based_actor* self,
                       const caf::group& result_group,
                       const caf::group& compare_result_group) {
  return {
      [=](int a, int b, int id) {
        aout(self) << "receive:" << a << " " << b << " " << id << " "
                   << std::endl;
        anon_send(result_group, a + b, id);
      },
      [=](int a, int b, int c, int id) {
        aout(self) << "receive:" << a << " " << b << " " << c << " " << id
                   << " " << std::endl;
        anon_send(result_group, a < b ? a + c : b + c, id);
      },
      [=](NumberCompareData& a) {
        if (0 == a.numbers.size()) {
          return;
        }

        aout(self) << "receive:" << a << std::endl;

        int result = a.numbers[0];
        for (int num : a.numbers) {
          if (num < result) {
            result = num;
          }
        }

        anon_send(compare_result_group, result, a.index);
      },

  };
}

void caf_main(caf::actor_system& system, const config& cfg) {
  auto result_group_exp = system.groups().get("remote", cfg.count_result_group);
  if (!result_group_exp) {
    std::cerr << "failed to get count result group: " << cfg.count_result_group
              << std::endl;
    return;
  }

  auto compare_group_exp = system.groups().get("remote", cfg.compare_group);
  if (!compare_group_exp) {
    std::cerr << "failed to get compare result group: " << cfg.compare_group
              << std::endl;
    return;
  }

  auto result_group = std::move(*result_group_exp);
  auto compare_group = std::move(*compare_group_exp);
  auto worker_actor = system.spawn(countAdd, result_group, compare_group);

  auto expected_port = caf::io::publish(worker_actor, cfg.worker_port);
  if (!expected_port) {
    std::cerr << "*** publish failed: " << system.render(expected_port.error())
              << std::endl;
    return;
  }

  std::cout << "worker started at port:" << expected_port << std::endl
            << "*** press [enter] to quit" << std::endl;
  std::string dummy;
  std::getline(std::cin, dummy);
  std::cout << "... cya" << std::endl;
}

CAF_MAIN(caf::io::middleman)
