//
// Created by Mingfei Deng on 2020/2/21.
//

#ifndef CDCF_YANGHUI_CONFIG_H
#define CDCF_YANGHUI_CONFIG_H
#include "caf/all.hpp"
#include "caf/io/all.hpp"

using namespace caf;
using namespace std;

struct NumberCompareData{
  vector<int> numbers;
  int index;
};

template <class Inspector >
typename Inspector::result_type inspect(Inspector& f, NumberCompareData& x) {
  return f(meta::type_name("NumberCompareData"), x.numbers, x.index);
}

struct foo {
    std::vector<int> a;
    int b;
};

// foo needs to be serializable
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, foo& x) {
    return f(meta::type_name("foo"), x.a, x.b);
}

class config : public actor_system_config {
 public:
  uint16_t port = 0;
  string host = "localhost";
  string worker_group = "";
  string count_result_group = "";
  string compare_group = "";
  uint16_t worker_port = 0;

  config() {
    opt_group{custom_options_, "global"}
        .add(port, "port,p", "set port")
        .add(host, "host,H", "set node (ignored in server mode)")
        .add(worker_group, "group,g", "set worker group name@host:port")
        .add(count_result_group, "result,r", "set count result group name@host:port")
        .add(compare_group, "compare, c", "set compare result group name@host:port")
        .add(worker_port, "instance, i", "set instance id");
    add_message_type<NumberCompareData>("NumberCompareData");
    add_message_type<foo>("foo");
  }
};




#endif  // CDCF_YANGHUI_CONFIG_H
