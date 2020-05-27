/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../actor_fault_tolerance/include/actor_union.h"
#include "../../config_manager/include/cdcf_config.h"

using add_atom = caf::atom_constant<caf::atom("add")>;
using sub_atom = caf::atom_constant<caf::atom("sub")>;

using calculator =
    caf::typed_actor<caf::replies_to<add_atom, int, int>::with<int>,
                     caf::replies_to<sub_atom, int, int>::with<int>>;

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](add_atom, int a, int b) -> int {
            caf::aout(self)
                << "received add_atom task from remote node. input a:" << a
                << " b:" << b << std::endl;
            return a + b;
          },
          [=](sub_atom, int a, int b) -> int {
            caf::aout(self)
                << "received sub_atom task from a remote node. input a:" << a
                << " b:" << b << std::endl;
            return a - b;
          }};
}

void printRet(int return_value) {
  std::cout << "return value:" << return_value << std::endl;
}

void dealSendErr(const caf::error& err) {
  std::cout << "get error:" << caf::to_string(err) << std::endl;
}

class config : public CDCFConfig {
 public:
  std::string host = "localhost";
  uint16_t worker_number = 0;

  config() {
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(host, "host,H", "set node")
        .add(worker_number, "worker_number,w", "set worker number");
  }

  uint16_t worker_ports[3] = {56098, 56099, 56100};
};

void Worker(caf::actor_system& system, const config& cfg) {
  auto res = system.middleman().open(cfg.worker_ports[cfg.worker_number - 1]);
  if (!res) {
    std::cerr << "*** cannot open port: " << system.render(res.error())
              << std::endl;
    return;
  }
  std::cout << "*** running on port: " << *res << std::endl
            << "*** press <enter> to shutdown worker" << std::endl;
  getchar();
}

caf::actor StartWorker(caf::actor_system& system, const caf::node_id& nid,
                       const std::string& name, caf::message args,
                       std::chrono::seconds timeout, bool& active) {
  auto worker = system.middleman().remote_spawn<calculator>(
      nid, name, std::move(args), timeout);
  if (!worker) {
    std::cerr << "*** remote spawn failed: " << system.render(worker.error())
              << std::endl;

    active = false;
    return caf::actor{};
  }

  auto ret_actor = caf::actor_cast<caf::actor>(std::move(*worker));
  active = true;

  return ret_actor;
}

void UnionLeader(caf::actor_system& system, const config& cfg) {
  auto node1 = system.middleman().connect(cfg.host, cfg.worker_ports[0]);
  if (!node1) {
    std::cerr << "*** connect failed: " << system.render(node1.error())
              << std::endl;
    return;
  }

  auto node2 = system.middleman().connect(cfg.host, cfg.worker_ports[1]);
  if (!node2) {
    std::cerr << "*** connect failed: " << system.render(node1.error())
              << std::endl;
    return;
  }

  auto node3 = system.middleman().connect(cfg.host, cfg.worker_ports[2]);
  if (!node3) {
    std::cerr << "*** connect failed: " << system.render(node1.error())
              << std::endl;
    return;
  }

  auto type = "calculator";              // type of the actor we wish to spawn
  auto args = caf::make_message();       // arguments to construct the actor
  auto tout = std::chrono::seconds(30);  // wait no longer than 30s

  bool active = true;
  auto worker_actor1 = StartWorker(system, *node1, type, args, tout, active);
  if (!active) {
    std::cout << "start work actor1 failed" << std::endl;
    return;
  }

  auto worker_actor2 = StartWorker(system, *node2, type, args, tout, active);
  if (!active) {
    std::cout << "start work actor2 failed" << std::endl;
    return;
  }

  auto worker_actor3 = StartWorker(system, *node3, type, args, tout, active);
  if (!active) {
    std::cout << "start work actor3 failed" << std::endl;
    return;
  }

  ActorUnion actor_union(system, caf::actor_pool::round_robin());

  actor_union.AddActor(worker_actor1);
  actor_union.AddActor(worker_actor2);
  actor_union.AddActor(worker_actor3);

  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 1, 600);
  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 2, 600);
  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 3, 600);

  // let actor1 down
  caf::anon_send_exit(worker_actor1, caf::exit_reason::kill);

  // let actor1 down
  // caf::anon_send_exit(worker_actor3, caf::exit_reason::kill);

  // remove actor2 down
  // actor_union.RemoveActor(worker_actor2);

  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 1, 990);
  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 2, 990);
  actor_union.SendAndReceive(printRet, dealSendErr, add_atom::value, 3, 990);
}

void caf_main(caf::actor_system& system, const config& cfg) {
  auto f = (0 != cfg.worker_number) ? Worker : UnionLeader;
  f(system, cfg);
}

CAF_MAIN(caf::io::middleman)
