//
// Created by Mingfei Deng on 2020/3/6.
//

#include "yanghui_config.h"


behavior calculator_fun(event_based_actor* self){
  return {
      [=](int a, int b, int id)->int{
        aout(self) << "+++++" << endl;
        aout(self) << a << b << id << endl;
        return 5;
      }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  auto calc = system.spawn(calculator_fun);
  //cout << "*** try publish at port " << cfg.port << endl;
  auto expected_port = io::publish(calc, 51563);
  if (!expected_port) {
    std::cerr << "*** publish failed: "
              << system.render(expected_port.error()) << endl;
    return;
  }
  cout << "*** server successfully published at port " << *expected_port << endl
       << "*** press [enter] to quit" << endl;
  string dummy;
  std::getline(std::cin, dummy);
  cout << "... cya" << endl;
  anon_send_exit(calc, exit_reason::user_shutdown);
}

CAF_MAIN(io::middleman)