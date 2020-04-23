/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include <string>

#include <caf/all.hpp>
#include <caf/io/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
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

class config : public CDCFConfig {
 public:
  std::string host = "localhost";
  uint16_t port = 56088;
  bool worker_mode = false;

  config() {
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node")
        .add(worker_mode, "guard_mode,s", "enable guard mode");
  }
};

void Worker(caf::actor_system& system, const config& cfg) {
  auto res = system.middleman().open(cfg.port);
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

void printRet(int return_value) {
  std::cout << "return value:" << return_value << std::endl;
}

void dealSendErr(const caf::error& err) {
  std::cout << "get error:" << caf::to_string(err) << std::endl;
}

void UnionLeader(caf::actor_system& system, const config& cfg) {
  auto node = system.middleman().connect(cfg.host, cfg.port);
  if (!node) {
    std::cerr << "*** connect failed: " << system.render(node.error())
              << std::endl;
    return;
  }
  auto type = "calculator";              // type of the actor we wish to spawn
  auto args = caf::make_message();       // arguments to construct the actor
  auto tout = std::chrono::seconds(30);  // wait no longer than 30s

  bool active = true;
  auto worker_actor = StartWorker(system, *node, type, args, tout, active);
  if (!active) {
    std::cout << "start work actor failed" << std::endl;
    return;
  }

  ActorGuard actor_guard(
      worker_actor,
      [&](bool active) {
        return StartWorker(system, *node, type, args, tout, active);
      },
      system);

  caf::scoped_actor self{system};

  std::cout << "*** press <enter> to show normal send message" << std::endl;
  getchar();

  // normal send message
  self->request(worker_actor, std::chrono::seconds(1), add_atom::value, 200,
                100)
      .receive([](int ret) { std::cout << "get ret:" << ret << std::endl; },
               dealSendErr);

  std::cout << "*** press <enter> to show actor guard send message"
            << std::endl;
  getchar();

  // guard send message
  actor_guard.SendAndReceive(printRet, dealSendErr, add_atom::value, 50, 100);

  std::cout << "*** press <enter> to let actor down." << std::endl;
  getchar();

  // let actor down
  caf::anon_send_exit(worker_actor, caf::exit_reason::kill);

  std::cout << "*** press <enter> to show normal send message after actor down."
            << std::endl;
  getchar();

  // normal send message
  self->request(worker_actor, std::chrono::seconds(1), add_atom::value, 500,
                100)
      .receive([](int ret) { std::cout << "get ret:" << ret << std::endl; },
               dealSendErr);

  std::cout << "*** press <enter> to show guard send after message down."
            << std::endl;
  getchar();

  // guard send message after error
  actor_guard.SendAndReceive(printRet, dealSendErr, add_atom::value, 66, 600);

  std::cout << "*** press <enter> to shutdown guard" << std::endl;
  getchar();
}

void caf_main(caf::actor_system& system, const config& cfg) {
  auto f = cfg.worker_mode ? UnionLeader : Worker;
  f(system, cfg);
}

CAF_MAIN(caf::io::middleman)
