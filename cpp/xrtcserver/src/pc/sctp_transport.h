#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include <rtc_base/third_party/sigslot/sigslot.h>

#include "pc/sctp_session.h"

namespace xrtc {

class SctpTransport : public sigslot::has_slots<> {
public:
    using OnDataReceived = std::function<void(void* data, size_t len)>;
    using OnStateChange = std::function<void(SctpSession::State)>;

    SctpTransport();
    virtual ~SctpTransport();

    bool start();
    void stop();
    
    bool send_data(const char* data, size_t len);
    
    SctpSession::State state() const;
    
    // 设置回调
    void set_on_send_to_local_(OnDataReceived callback);
    void set_on_sctp_data_received(OnDataReceived callback);
    void set_on_state_change(OnStateChange callback);
    
    // 处理接收到的数据
    void handle_data_received(const char* data, size_t len);

private:
    void _create_sctp_session();

    std::unique_ptr<SctpSession> _session;
};

} // namespace xrtc