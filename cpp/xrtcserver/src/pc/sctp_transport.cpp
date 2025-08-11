#include "pc/sctp_transport.h"
#include <rtc_base/logging.h>

namespace xrtc {

SctpTransport::SctpTransport() {
    _create_sctp_session();
}

SctpTransport::~SctpTransport() {
    stop();
}

bool SctpTransport::start() {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "SCTP session not created";
        return false;
    }
    
    return _session->start();
}

void SctpTransport::stop() {
    if (_session) {
        _session->stop();
    }
}

bool SctpTransport::send_data(const char* data, size_t len) {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "SCTP session not created";
        return false;
    }
    
    return _session->send_data(data, len);
}

SctpSession::State SctpTransport::state() const {
    if (!_session) return SctpSession::kNew;
    return _session->state();
}

void SctpTransport::set_on_send_to_local_(OnDataReceived callback) {
    if (_session) {
        _session->set_on_send_to_local_(callback);
    }
}

void SctpTransport::set_on_sctp_data_received(OnDataReceived callback) {
    if (_session) {
        _session->set_on_sctp_data_received(callback);
    }
}

void SctpTransport::set_on_state_change(OnStateChange callback) {
    if (_session) {
        _session->set_on_state_change(callback);
    }
}

void SctpTransport::handle_data_received(const char* data, size_t len) {
    if (_session) {
        _session->handle_data_received(data, len);
    }
}

void SctpTransport::_create_sctp_session() {
    _session = std::make_unique<SctpSession>();
}

} // namespace xrtc