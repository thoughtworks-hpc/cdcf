/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */

#include "../include/config_manager_test.h"

#include <gmock/gmock.h>

#include "cdcf/cdcf_config.h"

class test_config : public cdcf::CDCFConfig {
 public:
  uint16_t my_port = 0;
  bool my_server_mode = false;

  test_config() {
    opt_group{custom_options_, "global"}
        .add(my_port, "port,p", "set port")
        .add(my_server_mode, "server-mode,s", "enable server mode");
  }
};

TEST(TestConfigLoad, load_from_file) { /* NOLINT */
  int fake_argc = 2;
  char arg0[] = "test_program";

  char **fake_argv = new char *[fake_argc + 1] { arg0, INI_FILE_PARAMETER };

  test_config config;
  cdcf::CDCFConfig::RetValue ret =
      config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

  EXPECT_THAT(cdcf::CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_EQ(8089, config.my_port);
  EXPECT_EQ("localhost111", config.host_);
  EXPECT_EQ(true, config.my_server_mode);
}

TEST(TestConfigLoad, load_from_parameter) { /* NOLINT */
  int fake_argc = 4;
  char arg0[] = "test_program";
  char arg1[] = "--port=8088";
  char arg2[] = "--host=localhost666";
  char arg3[] = "--server-mode";

  char **fake_argv = new char *[fake_argc + 1] { arg0, arg1, arg2, arg3 };

  test_config config;
  cdcf::CDCFConfig::RetValue ret =
      config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

  EXPECT_THAT(cdcf::CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_EQ(8088, config.my_port);
  EXPECT_EQ("localhost666", config.host_);
  EXPECT_EQ(true, config.my_server_mode);
}

TEST(TestConfigLoad, load_from_parameter_short) { /* NOLINT */
  int fake_argc = 6;
  char arg0[] = "test_program";
  char arg1[] = "-p";
  char arg2[] = "8088";
  char arg3[] = "-H";
  char arg4[] = "localhost666";
  char arg5[] = "-s";

  char **fake_argv =
      new char *[fake_argc + 1] { arg0, arg1, arg2, arg3, arg4, arg5 };

  test_config config;
  cdcf::CDCFConfig::RetValue ret =
      config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

  EXPECT_THAT(cdcf::CDCFConfig::RetValue::kSuccess, ret);
  EXPECT_EQ(8088, config.my_port);
  EXPECT_EQ("localhost666", config.host_);
  EXPECT_EQ(true, config.my_server_mode);
}
