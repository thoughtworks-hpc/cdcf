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
  int message_count = 0;
  return {[&](int a, int b, int id) {
            message_count++;
            aout(self) << "receive:" << a << " " << b << " " << id << " "
                       << "message_count:" << message_count << std::endl;
            caf::message_builder msg_builder;
            msg_builder.append_all(a + b, id);

            std::this_thread::sleep_for(std::chrono::seconds(load_second));
            return msg_builder.to_message();
            // anon_send(result_actor, a + b, id);
          },
          [&](int a, int b, int c, int id) {
            message_count++;
            aout(self) << "receive:" << a << " " << b << " " << c << " " << id
                       << ". message_count:" << message_count << std::endl;

            caf::message_builder msg_builder;
            msg_builder.append_all(a < b ? a + c : b + c, id);

            std::this_thread::sleep_for(std::chrono::seconds(load_second));
            return msg_builder.to_message();
          },
          [&](NumberCompareData& a) {
            if (a.numbers.empty()) {
              caf::message_builder msg_builder;
              return msg_builder.to_message();
            }

            message_count++;
            aout(self) << "receive:" << a << ". message_count:" << message_count
                       << std::endl;

            int result = a.numbers[0];
            for (int num : a.numbers) {
              if (num < result) {
                result = num;
              }
            }

            caf::message_builder msg_builder;
            msg_builder.append_all(result, a.index);

            std::this_thread::sleep_for(std::chrono::seconds(load_second));
            return msg_builder.to_message();
          }};
}
