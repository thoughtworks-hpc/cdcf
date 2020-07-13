/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "../../actor_system/include/actor_status_monitor.h"
#include "../../actor_system/include/actor_status_service_grpc_impl.h"
#include "../../config_manager/include/cdcf_config.h"

class config : public CDCFConfig {
 public:
  std::string host = "localhost";
  uint16_t port = 50052;
  config() {
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node");
  }
};

caf::behavior simple_counter(caf::event_based_actor* self) {
  return {[=](int a, int b) {
    aout(self) << "receive:" << a << "+" << b << std::endl;
    int result = a + b;
    aout(self) << "result:" << result << std::endl;
    return result;
  }};
}

void caf_main(caf::actor_system& system, const config& cfg) {
  ActorStatusMonitor actor_status_monitor(system);
  ActorStatusServiceGprcImpl actor_status_(system, actor_status_monitor,
                                           cfg.port);

  auto add_actor1 = system.spawn(simple_counter);
  actor_status_monitor.RegisterActor(add_actor1, "Adder1",
                                     "a actor can count add.");

  auto add_actor2 = system.spawn(simple_counter);
  actor_status_monitor.RegisterActor(add_actor2, "Adder2",
                                     "a actor can count add.");

  auto add_actor3 = system.spawn(simple_counter);
  actor_status_monitor.RegisterActor(add_actor3, "Adder3",
                                     "a actor can count add.");

  std::cout << "server start at port:" << cfg.port << std::endl;

  actor_status_.RunWithWait();
}

CAF_MAIN(caf::io::middleman)
