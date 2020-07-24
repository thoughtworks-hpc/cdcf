/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include <cdcf_config.h>
#include <gmock/gmock.h>

#include <cmath>

TEST(TestScheduler, threads_proportion) {
  std::vector<char *> argv{(char *)("scheduler_test"),
                           (char *)("--threads_proportion=0.52")};
  int argc = static_cast<int>(argv.size());
  for (int i = 0; i < argc; i++) {
    argv[i] = argv[i];
  }

  CDCFConfig config;
  CDCFConfig::RetValue ret =
      config.parse_config(argc, argv.data(), "cdcf-application.ini");

  auto thread_num = static_cast<size_t>(std::thread::hardware_concurrency() *
                                        config.threads_proportion);
  config.set("scheduler.max-threads", thread_num);

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_EQ(0.52, config.threads_proportion);
  EXPECT_EQ(thread_num, sch.num_workers());
}

TEST(TestScheduler, load_from_file) {
  std::vector<char *> argv{(char *)("scheduler_test")};
  int argc = static_cast<int>((argv.size()));
  for (int i = 0; i < argc; i++) {
    argv[i] = argv[i];
  }

  CDCFConfig config;
  CDCFConfig::RetValue ret =
      config.parse_config(argc, argv.data(), "cdcf-application.ini");

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_TRUE(floor(std::thread::hardware_concurrency() *
                    config.threads_proportion) == sch.num_workers() ||
              4 == sch.num_workers());
}

TEST(TestScheduler, load_default) {
  std::vector<char *> argv{(char *)("scheduler_test")};
  int argc = static_cast<int>(argv.size());
  for (int i = 0; i < argc; i++) argv[i] = argv[i];

  CDCFConfig config;
  CDCFConfig::RetValue ret = config.parse_config(argc, argv.data());

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_TRUE(std::thread::hardware_concurrency() == sch.num_workers() ||
              4 == sch.num_workers());
}

TEST(TestScheduler, load_option) {
  std::vector<char *> argv{
      (char *)("scheduler_test"),
      (char *)("--scheduler.max-threads=7"),
      (char *)("--scheduler.policy=stealing"),
      (char *)("--scheduler.profiling-output-file=/home"),
  };
  int argc = static_cast<int>(argv.size());

  CDCFConfig config;
  CDCFConfig::RetValue ret = config.parse_config(argc, argv.data());

  caf::actor_system system{config};
  caf::scheduler::abstract_coordinator &sch = system.scheduler();

  EXPECT_THAT(CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_TRUE(floor(std::thread::hardware_concurrency() *
                    config.threads_proportion) == sch.num_workers() ||
              4 * config.threads_proportion == sch.num_workers());
  EXPECT_EQ("/home",
            caf::get_or(config, "scheduler.profiling-output-file", ""));
}
