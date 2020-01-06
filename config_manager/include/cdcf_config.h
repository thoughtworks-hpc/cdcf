//
// Created by Mingfei Deng on 2019/12/26.
//

#ifndef CDCF_CDCF_CONFIG_H
#define CDCF_CDCF_CONFIG_H

#include "caf/all.hpp"
//#include "caf/io/all.hpp"


class cdcf_config : public caf::actor_system_config {
public:
    cdcf_config(){};

    cdcf_config(cdcf_config &&config) = default;
    cdcf_config(const cdcf_config &config) = delete;
    cdcf_config& operator=(const cdcf_config&) = delete;

    virtual ~cdcf_config(){};

    enum RET_VALUE{
        SUCCESS = 0, FAILURE = 1,
    };

    RET_VALUE parse_config(int argc, char** argv,
                               const char* ini_file_cstr = "cdcf-default.ini");
};


#endif //CDCF_CDCF_CONFIG_H
