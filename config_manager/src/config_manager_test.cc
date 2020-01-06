#include <gmock/gmock.h>
#include "cdcf_config.h"
#include "config_manager_test.h"


class test_config :public cdcf_config{

public:
    uint16_t my_port = 0;
    std::string my_host = "localhost";
    bool my_server_mode = false;

    test_config() {
        opt_group{custom_options_, "global"}
                .add(my_port, "port,p", "set port")
                .add(my_host, "host,H", "set node (ignored in server mode)")
                .add(my_server_mode, "server-mode,s", "enable server mode");
    }
};


TEST(TestConfigLoad, load_from_file)
{
    int fake_argc = 2 ;
    char arg0[] = "test_program" ;
    const std::string ini_file_path = "--config-file=" + INI_FILE_PATH;
    //char arg1[] = ini_file_path.c_str();
    //char *arg1 = new char[ini_file_path.size()];
    char arg1[ini_file_path.size()+1];
    std::strcpy(arg1, ini_file_path.c_str());

    char **fake_argv = new char*[fake_argc+1]{ arg0 , arg1 } ;

    test_config config;
    cdcf_config::RET_VALUE ret = config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

    EXPECT_THAT(0, ret);
    EXPECT_EQ(8089, config.my_port);
    EXPECT_EQ("localhost111", config.my_host);
    EXPECT_EQ(true, config.my_server_mode);
}

TEST(TestConfigLoad, load_from_parameter)
{
    int fake_argc = 4 ;
    char arg0[] = "test_program";
    char arg1[] = "--port=8088";
    char arg2[] = "--host=localhost666";
    char arg3[] = "--server-mode";

    char **fake_argv = new char*[fake_argc+1]{arg0, arg1, arg2, arg3};

    test_config config;
    cdcf_config::RET_VALUE ret = config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

    EXPECT_THAT(0, ret);
    EXPECT_EQ(8088, config.my_port);
    EXPECT_EQ("localhost666", config.my_host);
    EXPECT_EQ(true, config.my_server_mode);
}

TEST(TestConfigLoad, load_from_parameter_short)
{
    int fake_argc = 6 ;
    char arg0[] = "test_program";
    char arg1[] = "-p";
    char arg2[] = "8088";
    char arg3[] = "-H";
    char arg4[] = "localhost666";
    char arg5[] = "-s";

    char **fake_argv = new char*[fake_argc+1]{arg0, arg1, arg2, arg3, arg4, arg5};

    test_config config;
    cdcf_config::RET_VALUE ret = config.parse_config(fake_argc, fake_argv, "cdcf-default.ini");

    EXPECT_THAT(0, ret);
    EXPECT_EQ(8088, config.my_port);
    EXPECT_EQ("localhost666", config.my_host);
    EXPECT_EQ(true, config.my_server_mode);
}