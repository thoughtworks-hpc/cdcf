/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/simple_counter.h"

caf::behavior simple_counter(caf::event_based_actor* self) {
  return {[=](int a, int b, int id) {
            aout(self) << "receive:" << a << " " << b << " " << id << " "
                       << std::endl;
            caf::message_builder msg_builder;
            msg_builder.append_all(a + b, id);

            return msg_builder.to_message();
            // anon_send(result_actor, a + b, id);
          },
          [=](int a, int b, int c, int id) {
            aout(self) << "receive:" << a << " " << b << " " << c << " " << id
                       << " " << std::endl;

            caf::message_builder msg_builder;
            msg_builder.append_all(a < b ? a + c : b + c, id);

            return msg_builder.to_message();
          },
          [=](NumberCompareData& a) {
            if (a.numbers.empty()) {
              caf::message_builder msg_builder;
              return msg_builder.to_message();
            }

            aout(self) << "receive:" << a << std::endl;

            int result = a.numbers[0];
            for (int num : a.numbers) {
              if (num < result) {
                result = num;
              }
            }

            caf::message_builder msg_builder;
            msg_builder.append_all(result, a.index);

            return msg_builder.to_message();
          }};
}

caf::behavior simple_counter_add_load(caf::event_based_actor* self,
                                      int load_second) {
  static int message_count = 0;
  std::cout << "start sleep:" << load_second << std::endl;
  return {[=](int a, int b, int id, caf::actor result_actor) {
            //            message_count++;
            std::cout << "receive:" << a << " " << b << " " << id << " "
                      << "message_count:" << message_count++ << std::endl;
            caf::message_builder msg_builder;
            msg_builder.append_all(a + b, id);

            std::cout << "sleep:" << load_second << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(load_second));

            anon_send(result_actor, a + b, id);
            // return msg_builder.to_message();
            return 0;
          },
          [=](int a, int b, int c, int id, caf::actor result_actor) {
            //            message_count++;
            std::cout << "receive:" << a << " " << b << " " << c << " " << id
                      << "message_count:" << message_count++ << std::endl;

            caf::message_builder msg_builder;
            msg_builder.append_all(a < b ? a + c : b + c, id);

            std::cout << "sleep:" << load_second << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(load_second));

            anon_send(result_actor, a < b ? a + c : b + c, id);
            return 0;
          },
          [=](NumberCompareData& a, caf::actor& result_actor) {
            if (a.numbers.empty()) {
              //              caf::message_builder msg_builder;
              // return 0;
            }

            //            message_count++;
            std::cout << "receive:" << a << " message_count:" << message_count++
                      << std::endl;

            int result = a.numbers[0];
            for (int num : a.numbers) {
              if (num < result) {
                result = num;
              }
            }

            caf::message_builder msg_builder;
            msg_builder.append_all(result, a.index);

            std::cout << "sleep:" << load_second << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(load_second));

            // anon_send(result_actor, result, a.index);
            return msg_builder.to_message();
            // return 0;
          }};
}
