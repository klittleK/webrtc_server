#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <map>

#include <usrsctp/usrsctp.h>

#include "pc/datachannel_processor.h"

#define DATA_CHANNEL_PPID_CONTROL          (50)     //控制消息
#define DATA_CHANNEL_PPID_STRING           (51)     //字符串数据
#define DATA_CHANNEL_PPID_BINARY           (53)     //二进制数据

namespace xrtc {

// 分片重组结构
struct FragmentedMessage {
    uint16_t stream_id;
    uint32_t ppid;
    std::vector<uint8_t> fragments;
    bool is_complete = false;
};


class SctpSession {
public:
    enum State {
        kNew,
        kConnecting,
        kConnected,
        kClosed,
        kFailed
    };
    using OnDataReceived = std::function<void(void* data, size_t len)>;
    using OnStateChange = std::function<void(State)>;

    SctpSession();
    ~SctpSession();
    
    bool init();
    void shutdown();
    
    bool start();
    void stop();
    
    bool send_data(const char* data, size_t len);
    
    State state() const { return _state; }
    
    // 设置回调
    void set_on_send_to_local_(OnDataReceived callback) { on_send_to_local_ = callback; }
    void set_on_sctp_data_received(OnDataReceived callback) { on_sctp_data_received_ = callback; }
    void set_on_state_change(OnStateChange callback) { on_state_change_ = callback; }
    
    // 处理接收到的数据
    void handle_data_received(const char* data, size_t len);
    bool handle_output(void* buffer, size_t length);

private:
    void set_state(State state);
    static int on_sctp_event(struct socket* sock, union sctp_sockstore addr, void* data, size_t datalen,
                            struct sctp_rcvinfo rcv, int flags, void* user_data);
    static int on_send_data(struct socket* sock, uint32_t sb_free, void* ulp_info);
    static void on_remoteup(struct socket* sock, void* arg, int flgs);
    void SendDatachannelControlResponse(const std::vector<uint8_t>& response, uint16_t stream_id, uint32_t ppid);
    void ForwardApplicationData(const uint8_t* data, size_t len);
    
    State _state = kNew;
    struct socket* _local_socket = nullptr;
    struct socket* _remote_socket = nullptr;
    OnDataReceived on_send_to_local_;
    OnDataReceived on_sctp_data_received_;
    OnStateChange on_state_change_;
    bool _initialized = false;
    int _local_port = 5000;

    DataChannelProcessor _processor;
    std::map<uint32_t, FragmentedMessage> fragmented_messages_; // TSN -> 分片消息
};

} // namespace xrtc