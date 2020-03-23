/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include <cdcf_config.h>
#include <gmock/gmock.h>
#include <math.h>

TEST(TestScheduler, threads_proportion) {
  char argv[][64] = {"scheduler_test", "--threads_proportion=0.52"};
  int argc = sizeof(argv) / sizeof(argv[0]);
  char *argv_ptr[argc];

  for (int i = 0; i < argc; i++) argv_ptr[i] = argv[i];

  cdcf_config config;
  cdcf_config::RET_VALUE ret =
      config.parse_config(argc, argv_ptr, "cdcf-application.ini");

  size_t thread_num =
      floor(std::thread::hardware_concurrency() * config.threads_proportion);
  config.set("scheduler.max-threads", thread_num);

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(0, ret);
  EXPECT_EQ(0.52, config.threads_proportion);
  EXPECT_EQ(thread_num, sch.num_workers());
}

TEST(TestScheduler, load_from_file) {
  char argv[][64] = {"scheduler_test"};
  int argc = sizeof(argv) / sizeof(argv[0]);
  char *argv_ptr[argc];

  for (int i = 0; i < argc; i++) argv_ptr[i] = argv[i];

  cdcf_config config;
  cdcf_config::RET_VALUE ret =
      config.parse_config(argc, argv_ptr, "cdcf-application.ini");

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(0, ret);
  EXPECT_TRUE(floor(std::thread::hardware_concurrency() *
                    config.threads_proportion) == sch.num_workers() ||
              4 * config.threads_proportion == sch.num_workers());
}

TEST(TestScheduler, load_default) {
  char argv[][64] = {"scheduler_test"};
  int argc = sizeof(argv) / sizeof(argv[0]);
  char *argv_ptr[argc];

  for (int i = 0; i < argc; i++) argv_ptr[i] = argv[i];

  cdcf_config config;
  cdcf_config::RET_VALUE ret = config.parse_config(argc, argv_ptr);

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(0, ret);
  EXPECT_TRUE(std::thread::hardware_concurrency() == sch.num_workers() ||
              4 == sch.num_workers());
}

TEST(TestScheduler, load_option) {
  char argv[][64] = {"scheduler_test", "--scheduler.max-threads=7",
                     "--scheduler.policy=stealing",
                     "--scheduler.profiling-output-file=/home"};
  int argc = sizeof(argv) / sizeof(argv[0]);
  char *argv_ptr[argc];

  for (int i = 0; i < argc; i++) argv_ptr[i] = argv[i];

  cdcf_config config;
  cdcf_config::RET_VALUE ret = config.parse_config(argc, argv_ptr);

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(0, ret);
  EXPECT_TRUE(floor(std::thread::hardware_concurrency() *
                    config.threads_proportion) == sch.num_workers() ||
              4 * config.threads_proportion == sch.num_workers());
  EXPECT_EQ("/home",
            caf::get_or(config, "scheduler.profiling-output-file", ""));
}
