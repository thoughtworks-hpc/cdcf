//
// Created by Mingfei Deng on 2020/2/17.
//

#ifndef CDCF_ACTOR_SYSTEM_H
#define CDCF_ACTOR_SYSTEM_H
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "cdcf_config.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using ActorSystem = caf::actor_system;
using caf::atom_constant;
using caf::function_view;
using caf::replies_to;
using caf::typed_actor;
using caf::io::middleman;

template <size_t Size>
constexpr caf::atom_value Operation(char const (&str)[Size]) {
  return caf::atom(str);
}
template <caf::atom_value V>
struct OperationInfo {
  using Type = caf::atom_constant<V>;
};

#define SYSTEM_CONFIG(serverName, serverFun)                         \
  struct config : cdcf_config {                                      \
    config() {                                                       \
      add_actor_type((serverName), (serverFun));                     \
      opt_group{custom_options_, "global"}                           \
          .add(port, "port,p", "set port")                           \
          .add(host, "host,H", "set node (ignored in server mode)"); \
    }                                                                \
    uint16_t port = 0;                                               \
    string host = "localhost";                                       \
    bool server_mode = false;                                        \
  };

#define ACTOR_SYS_MAIN()                                                      \
  void caf_main(ActorSystem& system, const config& cfg) {                     \
    auto res = system.middleman().open(cfg.port);                             \
    if (!res) {                                                               \
      cerr << "*** cannot open port: " << system.render(res.error()) << endl; \
      return;                                                                 \
    }                                                                         \
    cout << "*** running on port: " << *res << endl                           \
         << "*** press <enter> to shutdown server" << endl;                   \
    getchar();                                                                \
  }                                                                           \
  CAF_MAIN(caf::io::middleman)

#define ACTOR_CLIENT_MAIN(clientFunction)                                     \
  void caf_main(ActorSystem& system, const config& cfg) {                     \
    auto node = system.middleman().connect(cfg.host, cfg.port);               \
    if (!node) {                                                              \
      cerr << "*** connect failed: " << system.render(node.error()) << endl;  \
      return;                                                                 \
    }                                                                         \
    cout << "connect to server host:" << cfg.host << " port:" << cfg.port     \
         << endl;                                                             \
    auto type = "calculator";                                                 \
    auto args = caf::make_message();                                          \
    auto tout = std::chrono::seconds(30);                                     \
    auto worker =                                                             \
        system.middleman().remote_spawn<calculator>(*node, type, args, tout); \
    if (!worker) {                                                            \
      cerr << "*** remote spawn failed: " << system.render(worker.error())    \
           << endl;                                                           \
      return;                                                                 \
    }                                                                         \
    clientFunction(make_function_view(*worker));                              \
    cout << "spawn actor:" << type << " success." << endl;                    \
    anon_send_exit(*worker, caf::exit_reason::kill);                          \
  }                                                                           \
  CAF_MAIN(caf::io::middleman)
#endif  // CDCF_ACTOR_SYSTEM_H
