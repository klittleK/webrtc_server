#pragma once

#include <string>

namespace xrtc {

struct GeneralConf {
    std::string log_dir;
    std::string log_name;
    std::string log_level;
    bool log_to_stderr;
    int ice_min_port = 0;
    int ice_max_port = 0;
};

int load_general_conf(const char* filename, GeneralConf* conf);

}