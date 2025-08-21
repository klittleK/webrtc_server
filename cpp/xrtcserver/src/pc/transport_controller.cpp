#include <rtc_base/logging.h>

#include "pc/transport_controller.h"
#include "pc/dtls_transport.h"
#include "pc/dtls_srtp_transport.h"
#include "pc/dtls_sctp_transport.h"


namespace xrtc {

TransportController::TransportController(EventLoop *el, PortAllocator* allocator):
    _el(el), 
    _ice_agent(new IceAgent(el, allocator))
{
    _ice_agent->signal_candidate_allocate_done.connect(this, &TransportController::on_candidate_allocate_done);
}

TransportController::~TransportController() {
    for (auto dtls_srtp : _dtls_srtp_transport_by_name) {
        delete dtls_srtp.second;
    }
    _dtls_srtp_transport_by_name.clear();

    for (auto dtls : _dtls_transport_by_name) {
        delete dtls.second;
    }
    _dtls_transport_by_name.clear();

    if (_ice_agent) {
        delete _ice_agent;
        _ice_agent = nullptr;
    }
}

int TransportController::set_local_description(SessionDescription *desc) {
    if (!desc) {
        RTC_LOG(LS_WARNING) << "desc is null";
        return -1;
    }
    
    // 查所有媒体段的mid，如果是在聚合中，只处理第一个的通道就行
    for (auto content : desc->contents()) {
        std::string mid = content->mid();
        if (desc->is_bundle(mid) && mid != desc->get_first_bundle_mid()) {
            continue;
        }

        if (content->type() == MediaType::MEDIA_TYPE_APPLICATION) {
            continue;
        }

        // 创建ICE通道
        _ice_agent->create_channel(_el, mid, IceCandidateComponent::RTP);
        auto td = desc->get_transport_info(mid);
        if (td) {
            _ice_agent->set_ice_params(mid, IceCandidateComponent::RTP, IceParameters(td->ice_ufrag, td->ice_pwd));
        }

        // 创建DTLS传输
        DtlsTransport* dtls = new DtlsTransport(_ice_agent->get_channel(mid, IceCandidateComponent::RTP));
        dtls->set_local_certificate(_local_certificate);
        dtls->signal_receiving_state.connect(this, &TransportController::_on_dtls_receiving_state);
        dtls->signal_receiving_state.connect(this, &TransportController::_on_dtls_writable_state);
        dtls->signal_dtls_state.connect(this, &TransportController::_on_dtls_state);
        _ice_agent->signal_ice_state.connect(this, &TransportController::_on_ice_state);
        _add_dtls_transport(dtls);

        // 处理DataChannel - 创建SCTP传输
        RTC_LOG(LS_INFO) << "Creating SCTP transport for DataChannel";

        // 创建SCTP传输
        DtlsSctpTransport* dtls_sctp = new DtlsSctpTransport(dtls->transport_name());
        dtls_sctp->set_dtls_transport(dtls);
        dtls_sctp->signal_sctp_data_received.connect(this, &TransportController::_on_datachannel_received);

        // 添加到SCTP传输列表
        _add_dtls_sctp_transport(dtls_sctp);
        DtlsSrtpTransport *dtls_srtp = new DtlsSrtpTransport(dtls->transport_name(), true);
        dtls_srtp->set_dtls_transports(dtls, nullptr);
        dtls_srtp->signal_rtp_packet_received.connect(this, &TransportController::_on_rtp_packet_received);
        dtls_srtp->signal_rtcp_packet_received.connect(this, &TransportController::_on_rtcp_packet_received);

        // 添加到DTLS SRTP传输控制器
        _add_dtls_srtp_transport(dtls_srtp);
    }

    _ice_agent->gathering_candidate();

    return 0; 
}

void TransportController::_on_rtp_packet_received(DtlsSrtpTransport*, rtc::CopyOnWriteBuffer* packet, int64_t ts) {
    signal_rtp_packet_received(this, packet, ts);
}

void TransportController::_on_rtcp_packet_received(DtlsSrtpTransport*, rtc::CopyOnWriteBuffer* packet, int64_t ts) {
    signal_rtcp_packet_received(this, packet, ts);
}

void TransportController::_on_datachannel_received(DtlsSctpTransport*, void* data, size_t len, int64_t ts) {
    RTC_LOG(LS_INFO) << "TransportController::_on_datachannel_received";
    signal_sctp_packet_received(this, data, len, ts);
}

void TransportController::_on_dtls_receiving_state(DtlsTransport*) {
    _update_state();
}

void TransportController::_on_dtls_writable_state(DtlsTransport*) {
    _update_state();
}

void TransportController::_on_dtls_state(DtlsTransport*, DtlsTransportState) {
    _update_state();
}

void TransportController::_on_ice_state(IceAgent*, IceTransportState) {
    _update_state();
}

void TransportController::_update_state() {
    PeerConnectionState pc_state = PeerConnectionState::k_new;
    std::map<DtlsTransportState, int> dtls_state_counts;
    std::map<IceTransportState, int> ice_state_counts;
    auto iter = _dtls_transport_by_name.begin();
    for (; iter != _dtls_transport_by_name.end(); ++iter) {
        dtls_state_counts[iter->second->dtls_state()]++;
        ice_state_counts[iter->second->ice_channel()->state()]++;
    }
    
    int total_connected = dtls_state_counts[DtlsTransportState::k_connected] + ice_state_counts[IceTransportState::k_connected];
    int total_dtls_connecting = dtls_state_counts[DtlsTransportState::k_connecting];
    int total_failed = dtls_state_counts[DtlsTransportState::k_failed] + ice_state_counts[IceTransportState::k_failed];
    int total_closed = dtls_state_counts[DtlsTransportState::k_closed] + ice_state_counts[IceTransportState::k_closed];
    int total_new = dtls_state_counts[DtlsTransportState::k_new] + ice_state_counts[IceTransportState::k_new];
    int total_ice_checking = ice_state_counts[IceTransportState::k_checking];
    int total_ice_disconnected = ice_state_counts[IceTransportState::k_disconnected];
    int total_ice_completed = ice_state_counts[IceTransportState::k_completed];

    int total_transports = _dtls_transport_by_name.size() * 2;

    if (total_failed > 0) {
        pc_state = PeerConnectionState::k_failed;
    } else if (total_ice_disconnected > 0) {
        pc_state = PeerConnectionState::k_disconnected;
    } else if (total_new + total_closed == total_transports) {
        pc_state = PeerConnectionState::k_new;
    } else if (total_ice_checking + total_dtls_connecting + total_new > 0) {
        pc_state = PeerConnectionState::k_connecting;
    } else if (total_connected + total_ice_completed + total_closed == total_transports) {
        pc_state = PeerConnectionState::k_connected;
    }

    if (_pc_state != pc_state) {
        _pc_state = pc_state;
        signal_connection_state(this, pc_state);
    }
}


void TransportController::_add_dtls_transport(DtlsTransport *dtls) {
    auto iter = _dtls_transport_by_name.find(dtls->transport_name());
    if (iter != _dtls_transport_by_name.end()) {
        delete iter->second; // 删除旧的
    }

    _dtls_transport_by_name[dtls->transport_name()] = dtls;
}

DtlsTransport* TransportController::_get_dtls_transport(const std::string& transport_name) {
    auto iter = _dtls_transport_by_name.find(transport_name);
    if (iter != _dtls_transport_by_name.end()) {
        return iter->second;
    }

    return nullptr;
}

void TransportController::_add_dtls_srtp_transport(DtlsSrtpTransport *dtls_srtp) {
    auto iter = _dtls_srtp_transport_by_name.find(dtls_srtp->transport_name());
    if (iter != _dtls_srtp_transport_by_name.end()) {
        delete iter->second; // 删除旧的
    }

    _dtls_srtp_transport_by_name[dtls_srtp->transport_name()] = dtls_srtp;
}

DtlsSrtpTransport* TransportController::_get_dtls_srtp_transport(const std::string& transport_name) {
    auto iter = _dtls_srtp_transport_by_name.find(transport_name);
    if (iter != _dtls_srtp_transport_by_name.end()) {
        return iter->second;
    }

    return nullptr;
}

void TransportController::_add_dtls_sctp_transport(DtlsSctpTransport *dtls_sctp) {
    auto iter = _dtls_sctp_transport_by_name.find(dtls_sctp->transport_name());
    if (iter != _dtls_sctp_transport_by_name.end()) {
        delete iter->second; // 删除旧的
    }

    _dtls_sctp_transport_by_name[dtls_sctp->transport_name()] = dtls_sctp;
}

DtlsSctpTransport* TransportController::_get_dtls_sctp_transport(const std::string& transport_name) {
    auto iter = _dtls_sctp_transport_by_name.find(transport_name);
    if (iter != _dtls_sctp_transport_by_name.end()) {
        return iter->second;
    }

    return nullptr;
}

int TransportController::set_remote_description(SessionDescription *desc) {
    if (!desc) {
        return -1;
    }

    for (auto content : desc->contents()) {
        std::string mid = content->mid();
        if (desc->is_bundle(mid) && mid != desc->get_first_bundle_mid()) {
            continue;
        }

        auto td = desc->get_transport_info(mid);
        if (td) {
            _ice_agent->set_remote_ice_params(
                content->mid(),
                IceCandidateComponent::RTP,
                IceParameters(td->ice_ufrag, td->ice_pwd));
            auto dtls = _get_dtls_transport(mid);
            if (dtls) {
                
                if (!td->identity_fingerprint) {
                    RTC_LOG(LS_INFO) << "mid: " << mid;
                    RTC_LOG(LS_ERROR) << "Identity fingerprint is null!";
                    continue;
                }

                // 检查指纹数据有效性
                const auto &digest = td->identity_fingerprint->digest;
                if (digest.size() == 0 || !digest.data())
                {
                    RTC_LOG(LS_ERROR) << "Empty DTLS fingerprint digest";
                }

                dtls->set_remote_fingerprint(td->identity_fingerprint->algorithm, (const uint8_t *)td->identity_fingerprint->digest.data(), td->identity_fingerprint->digest.size());
            }
        }
    }

    return 0;
}

void TransportController::set_local_certificate(rtc::RTCCertificate *cert) {
    _local_certificate = cert;
}

void TransportController::on_candidate_allocate_done(IceAgent *agent, const std::string &transport_name, IceCandidateComponent component, const std::vector<Candidate>& candidates) {
    signal_candidate_allocate_done(this, transport_name, component, candidates);
}

int TransportController::send_rtp(const std::string& transport_name, const char* data, size_t len) {
    auto dtls_srtp = _get_dtls_srtp_transport(transport_name);
    if (dtls_srtp) {
        return dtls_srtp->send_rtp(data, len);
    }
    return -1;
}

int TransportController::send_rtcp(const std::string& transport_name, const char* data, size_t len) {
    auto dtls_srtp = _get_dtls_srtp_transport(transport_name);
    if (dtls_srtp) {
        return dtls_srtp->send_rtcp(data, len);
    }
    return -1;
}

int TransportController::send_sctp(const std::string& transport_name, const char* data, size_t len) {
    auto dtls_sctp = _get_dtls_sctp_transport(transport_name);
    if (dtls_sctp) {
        return dtls_sctp->packet_and_send_dtls_sctp(data, len);
    }
    return -1;
}

}


