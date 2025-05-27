#pragma once

#define CMDNO_PUSH      1
#define CMDNO_PULL      2
#define CMDNO_ANSWER    3
#define CMDNO_STOP_PUSH 4
#define CMDNO_STOP_PULL 5

#define MAX_RES_BUF 4096

namespace xrtc {

struct RtcMsg {
    int cmdno = -1;
    uint64_t uid = 0;
    std::string stream_name;
    std::string stream_type;
    int audio = 0;
    int video = 0;
    uint32_t log_id = 0;

    void* worker = nullptr;
    void* conn = nullptr;
    int fd = 0;
    std::string sdp;
    int err_no = 0;
    void* certificate = nullptr;
};

}