#pragma once

#include <rtc_base/rtc_certificate.h>

#include "stream/rtc_stream.h"

namespace xrtc {

class PushStream : public RtcStream {

public:
    PushStream(EventLoop* el, PortAllocator* allocator, uint64_t uid,
        const std::string& stream_name, 
        bool audio, bool video, 
        uint32_t log_id);
    ~PushStream() override;

    std::string create_offer() override;
};






}