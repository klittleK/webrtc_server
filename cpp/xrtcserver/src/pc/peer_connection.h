#pragma once

#include <string>
#include <memory>

#include <base/event_loop.h>
#include <rtc_base/rtc_certificate.h>

#include "pc/session_description.h"
#include "pc/transport_controller.h"

namespace xrtc {

struct RTCOfferAnswerOptions {
    bool send_audio = true;
    bool send_video = true;
    bool recv_audio = true;
    bool recv_video = true;
    bool use_rtp_mux = true;
    bool use_rtcp_mux = true;
    bool dtls_on = true;
};

class PeerConnection : public sigslot::has_slots<> {

public:
    PeerConnection(EventLoop* el, PortAllocator* allocator);
    ~PeerConnection();

    int init(rtc::RTCCertificate* certificate);
    std::string creater_offer(const RTCOfferAnswerOptions& options);
    int set_remote_sdp(const std::string& sdp);

private:
    void on_candidate_allocate_done(TransportController* transport_controller, const std::string& transport_name, 
                                    IceCandidateComponent component, const std::vector<Candidate>& candidates);

private:
    EventLoop* _el;
    std::unique_ptr<SessionDescription> _local_desc;
    std::unique_ptr<SessionDescription> _remote_desc;
    rtc::RTCCertificate* _certificate = nullptr;
    std::unique_ptr<TransportController> _transport_controller;
};

}