#include <rtc_base/logging.h>

#include "pc/dtls_transport.h"
#include "dtls_transport.h"

namespace xrtc {

DtlsTransport::DtlsTransport(IceTransportChannel *ice_channel) :
    _ice_channel(ice_channel)
{
    _ice_channel->signal_read_packet.connect(this, &DtlsTransport::_on_read_packet);
}

DtlsTransport::~DtlsTransport()
{
}

void DtlsTransport::_on_read_packet(IceTransportChannel *channel, const char *buf, size_t len, int64_t ts) {
    // 处理DTLS数据包
}

}