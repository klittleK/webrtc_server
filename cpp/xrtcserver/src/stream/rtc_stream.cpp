#include <rtc_base/logging.h>

#include "rtc_stream.h"

namespace xrtc {

RtcStream::RtcStream(EventLoop *el, PortAllocator* allocator, uint64_t uid, const std::string &stream_name, bool audio, bool video, uint32_t log_id) :
    el(el), uid(uid), stream_name(stream_name), audio(audio), video(video), log_id(log_id), pc(new PeerConnection(el, allocator))
{
}

RtcStream::~RtcStream() {
}

int RtcStream::start(rtc::RTCCertificate *certificate) {
    return pc->init(certificate);
}

int RtcStream::set_remote_sdp(const std::string &sdp) {
    return pc->set_remote_sdp(sdp);
}
}