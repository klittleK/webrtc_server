#pragma once

#include <string>
#include <memory>

#include "pc/sctp_transport.h"
#include "pc/dtls_transport.h"

namespace xrtc {

class DtlsSctpTransport : public SctpTransport {
public:
    explicit DtlsSctpTransport(const std::string& transport_name);
    ~DtlsSctpTransport();
    
    void set_dtls_transport(DtlsTransport* dtls_transport);
    bool is_dtls_writable();
    const std::string& transport_name() const { return _transport_name; }
    int packet_and_send_dtls_sctp(const char* data, size_t len);
    int send_dtls_sctp(const char* data, size_t len);
    
    sigslot::signal4<DtlsSctpTransport*, void*, size_t, int64_t> signal_sctp_data_received;
    sigslot::signal2<DtlsSctpTransport*, SctpSession::State> signal_sctp_state_change;

private:
    void _maybe_setup_sctp();
    void _on_dtls_state(DtlsTransport* dtls, DtlsTransportState state);
    void _on_read_packet(DtlsTransport* dtls, const char* data, size_t len, int64_t ts);
    void _on_sctp_data_received(void* data, size_t len);
    void _on_send_to_local_(void* data, size_t len);
    void _on_sctp_state_change(SctpSession::State state);

    std::string _transport_name;
    DtlsTransport* _dtls_transport = nullptr;
    bool _sctp_active = false;
};

} // namespace xrtc