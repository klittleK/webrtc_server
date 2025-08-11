#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>

#include "pc/dtls_sctp_transport.h"

namespace xrtc {

DtlsSctpTransport::DtlsSctpTransport(const std::string& transport_name) :
    _transport_name(transport_name)
{
    // 设置 SCTP 回调
    set_on_send_to_local_([this](void* data, size_t len) {
        _on_send_to_local_(data, len);
    });

    set_on_sctp_data_received([this](void* data, size_t len) {
        _on_sctp_data_received(data, len);
    });
    
    set_on_state_change([this](SctpSession::State state) {
        _on_sctp_state_change(state);
    });
}

DtlsSctpTransport::~DtlsSctpTransport() {
    if (_dtls_transport) {
        _dtls_transport->signal_dtls_state.disconnect(this);
        _dtls_transport->signal_read_data_packet.disconnect(this);
    }
    RTC_LOG(LS_INFO) << "Destroyed DtlsSctpTransport: " << _transport_name;
}

void DtlsSctpTransport::set_dtls_transport(DtlsTransport* dtls_transport) {
    if (_dtls_transport == dtls_transport) {
        return;
    }
    
    if (_dtls_transport) {
        _dtls_transport->signal_dtls_state.disconnect(this);
        _dtls_transport->signal_read_data_packet.disconnect(this);
    }
    
    _dtls_transport = dtls_transport;
    
    if (_dtls_transport) {
        _dtls_transport->signal_dtls_state.connect(this, &DtlsSctpTransport::_on_dtls_state);
        _dtls_transport->signal_read_data_packet.connect(this, &DtlsSctpTransport::_on_read_packet);
        RTC_LOG(LS_INFO) << "Set DTLS transport for SCTP: " << _transport_name;
    }
}

bool DtlsSctpTransport::is_dtls_writable() {
    return _dtls_transport && _dtls_transport->writable();
}

int DtlsSctpTransport::packet_and_send_dtls_sctp(const char* data, size_t len) {
    // 传入原始用户数据，封装并发送sctp包
    if (send_data(data, len)) {
        return 0;
    }

    return -1;
}

int DtlsSctpTransport::send_dtls_sctp(const char* data, size_t len) {
    // 用于直接发封装好的sctp数据到外部
    return _dtls_transport->send_encrypted_data(data, len);
}

void DtlsSctpTransport::_on_dtls_state(DtlsTransport* dtls, DtlsTransportState state) {
    if (state == DtlsTransportState::k_connected) {
        RTC_LOG(LS_INFO) << "DTLS connected for SCTP: " << _transport_name;
        _maybe_setup_sctp();
    } else if (state == DtlsTransportState::k_closed || state == DtlsTransportState::k_failed) {
        RTC_LOG(LS_INFO) << "DTLS disconnected for SCTP: " << _transport_name;
        stop();
        _sctp_active = false;
    }
}

void DtlsSctpTransport::_on_read_packet(DtlsTransport* dtls, const char* data, size_t len, int64_t ts) {
    handle_data_received(data, len);
}

void DtlsSctpTransport::_on_send_to_local_(void* data, size_t len) {
    // 处理控制信号的响应，控制消息处理后回调函数同步传回给发来的用户
    int ret = send_dtls_sctp((const char*)data, len);
    if (ret < 0) {
        RTC_LOG(LS_WARNING) << " send sctp packet failed";
    } else {
        RTC_LOG(LS_WARNING) << " send sctp packet succeed";
    }
}

void DtlsSctpTransport::_on_sctp_data_received(void* data, size_t len) {
    // 触发收到了数据的信号，数据消息通过信号来异步转发
    RTC_LOG(LS_INFO) << "DtlsSctpTransport send sctp";
    
    signal_sctp_data_received(this, data, len, rtc::TimeMillis());
}

void DtlsSctpTransport::_on_sctp_state_change(SctpSession::State state) {
    // 更新 SCTP 活动状态
    if (state == SctpSession::kConnected) {
        _sctp_active = true;
    } else if (state == SctpSession::kClosed || state == SctpSession::kFailed) {
        _sctp_active = false;
    }
    
    // 触发状态变化信号
    signal_sctp_state_change(this, state);
}

void DtlsSctpTransport::_maybe_setup_sctp() {
    if (!is_dtls_writable() || _sctp_active) {
        return;
    }
    
    // 启动 SCTP 传输
    if (!start()) {
        RTC_LOG(LS_WARNING) << "Failed to start SCTP transport: ";
        return;
    }
}

} // namespace xrtc