#pragma once

#include <string>
#include <memory>

#include <base/event_loop.h>

#include "pc/peer_connection.h"

namespace xrtc {

class RtcStream {

public:
    RtcStream(EventLoop* el, PortAllocator* allocator, uint64_t uid,
        const std::string& stream_name, 
        bool audio, bool video, 
        uint32_t log_id);
    virtual ~RtcStream();

    int start(rtc::RTCCertificate* certificate);
    int set_remote_sdp(const std::string& sdp);

    virtual std::string create_offer() = 0;

protected:
    EventLoop* el;
    uint64_t uid;
    std::string stream_name;
    bool audio;
    bool video;
    uint32_t log_id;

    std::unique_ptr<PeerConnection> pc;

    friend class RtcStreamManager;
};

}