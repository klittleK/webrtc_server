#include <cstring>

#include <absl/base/attributes.h>
#include <rtc_base/synchronization/mutex.h>
#include <rtc_base/logging.h>

#include "pc/sctp_session.h"

namespace xrtc {

ABSL_CONST_INIT int g_libsctp_usage_count = 0;
ABSL_CONST_INIT webrtc::GlobalMutex g_libsctp_lock(absl::kConstInit);

// 辅助函数：解析并输出SCTP包内容（跳过数据段）
std::string format_sctp_packet(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    
    // 检查最小包长度
    const size_t MIN_PACKET_SIZE = 12; // 公共头长度
    if (len < MIN_PACKET_SIZE) {
        oss << "  [无效包，长度: " << len << " < " << MIN_PACKET_SIZE << "]";
        return oss.str();
    }
    
    // 解析公共头
    uint16_t source_port = ntohs(*reinterpret_cast<const uint16_t*>(data));
    uint16_t dest_port = ntohs(*reinterpret_cast<const uint16_t*>(data + 2));
    uint32_t verification_tag = ntohl(*reinterpret_cast<const uint32_t*>(data + 4));
    uint32_t checksum = ntohl(*reinterpret_cast<const uint32_t*>(data + 8));
    
    oss << "  SCTP公共头:\n"
        << "    源端口: " << source_port << "\n"
        << "    目的端口: " << dest_port << "\n"
        << "    验证标签: " << verification_tag << "\n"
        << "    校验和: " << checksum << "\n";
    
    // 解析chunks
    size_t offset = 12;
    size_t chunk_count = 0;
    
    while (offset + 4 <= len) {
        chunk_count++;
        uint8_t chunk_type = data[offset];
        uint8_t chunk_flags = data[offset + 1];
        uint16_t chunk_length = ntohs(*reinterpret_cast<const uint16_t*>(data + offset + 2));
        
        oss << "  Chunk #" << chunk_count << ":\n"
            << "    类型: " << static_cast<int>(chunk_type) << "\n"
            << "    标志: " << static_cast<int>(chunk_flags) << "\n"
            << "    长度: " << chunk_length << "\n";
        
        // 对于DATA chunk，只输出头部信息
        if (chunk_type == 0 && chunk_length >= 16) {
            oss << "    DATA chunk详情:\n";
            
            // 提取DATA chunk字段
            uint32_t tsn = ntohl(*reinterpret_cast<const uint32_t*>(data + offset + 4));
            uint16_t stream_id = ntohs(*reinterpret_cast<const uint16_t*>(data + offset + 8));
            uint16_t stream_seq = ntohs(*reinterpret_cast<const uint16_t*>(data + offset + 10));
            uint32_t ppid = ntohl(*reinterpret_cast<const uint32_t*>(data + offset + 12));
            
            oss << "      TSN: " << tsn << "\n"
                << "      流ID: " << stream_id << "\n"
                << "      序列号: " << stream_seq << "\n"
                << "      PPID: " << ppid << "\n"
                << "      数据长度: " << (chunk_length - 16) << " 字节\n";
        }
        
        // 移动到下一个chunk
        offset += chunk_length;
        
        // 对齐到4字节边界
        if (offset % 4 != 0) {
            offset += 4 - (offset % 4);
        }
        
        // 限制最多解析5个chunk
        if (chunk_count >= 5) {
            oss << "  [已解析前5个chunk，剩余部分省略...]";
            break;
        }
    }
    
    return oss.str();
}

int conn_output(void* addr, void* buffer, size_t length, uint8_t tos, uint8_t set_df) {
    RTC_LOG(LS_INFO) << "conn_output was called";
    SctpSession* self = static_cast<SctpSession*>(addr);
    if (self) {
        self->handle_output(buffer, length);
    }
    return 0;
}

SctpSession::SctpSession() {
    // 设置处理器回调
    _processor.SetControlResponseCallback([this](const std::vector<uint8_t>& response, uint16_t stream_id, uint32_t ppid) {
        this->SendDatachannelControlResponse(response, stream_id, ppid);
    });
    
    _processor.SetDataForwardCallback([this](const uint8_t* data, size_t len) {
        this->ForwardApplicationData(data, len);
    });
}

SctpSession::~SctpSession() {
    shutdown();
    usrsctp_deregister_address(this);
}

bool SctpSession::init() {
    if (_initialized) return true;
    
    webrtc::GlobalMutexLock ls(&g_libsctp_lock);

    if (g_libsctp_usage_count == 0) {
        usrsctp_init_nothreads(0, conn_output, nullptr);
        RTC_LOG(LS_INFO) << "usrsctp_init_nothreads Success!";
        usrsctp_sysctl_set_sctp_ecn_enable(0);
    }
    
    g_libsctp_usage_count++;
    _initialized = true;
    return true;
}

void SctpSession::shutdown() {
    if (!_initialized) return;
    
    stop();
    
    webrtc::GlobalMutexLock ls(&g_libsctp_lock);
    
    g_libsctp_usage_count--;
    if (g_libsctp_usage_count == 0) {
        usrsctp_finish();
        RTC_LOG(LS_INFO) << "usrsctp_finish called";
    }
    
    _initialized = false;
}

bool SctpSession::start() {
    init();
    usrsctp_register_address(this);
    _local_socket = usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP, on_sctp_event, on_send_data, 0, this);
    if (!_local_socket) {
        RTC_LOG(LS_INFO) << "usrsctp socket failed: " << std::strerror(errno);
        return false;
    }

    usrsctp_set_upcall(_local_socket, on_remoteup, this);

    struct sockaddr_conn sconn = {0};
    sconn.sconn_family = AF_CONN;
    sconn.sconn_port   = htons(5000);
    sconn.sconn_addr   = this;

    int ret = 0;
    ret = usrsctp_bind(_local_socket, (struct sockaddr *)&sconn, sizeof(sconn));
    if (ret < 0) {
        RTC_LOG(LS_INFO) << "usrsctp bind failed";
        return false;
    }

    usrsctp_set_non_blocking(_local_socket, 1);
 
    ret = usrsctp_listen(_local_socket, 1);
    if (ret < 0) {
        RTC_LOG(LS_ERROR) << "usrsctp_listen failed: " << strerror(errno);
        return false;
    }

    return true;
}

void SctpSession::stop() {
    if (_state == kClosed || _state == kFailed) {
        return;
    }
    
    if (_local_socket) {
        usrsctp_close(_local_socket);
        _local_socket = nullptr;
    }

    if (_remote_socket) {
        usrsctp_close(_remote_socket);
        _remote_socket = nullptr;
    }
    
    set_state(kClosed);
}

bool SctpSession::send_data(const char* data, size_t len) {
    RTC_LOG(LS_WARNING) << "call send data";
    if (_state != kConnected || !_remote_socket) {
        RTC_LOG(LS_WARNING) << "Cannot send data, SCTP not connected";
        return false;
    }

    // 使用 usrsctp 发送响应
    struct sctp_sndinfo sndinfo = {0};
    sndinfo.snd_sid = 0;
	sndinfo.snd_flags = SCTP_EOR;
	sndinfo.snd_ppid = htonl(DATA_CHANNEL_PPID_STRING);
	sndinfo.snd_context = 0;
	sndinfo.snd_assoc_id = 0;

    int ret = usrsctp_sendv(_remote_socket, data, len, NULL, 0, (void *)&sndinfo, (socklen_t)sizeof(sndinfo), SCTP_SENDV_SNDINFO, 0);
    if(0 > ret) {
        RTC_LOG(LS_WARNING) << "usrsctp_sendv failed " << ret;
    }

    return true;
}

void SctpSession::set_state(State state) {
    if (_state != state) {
        _state = state;
        if (on_state_change_) {
            on_state_change_(state);
        }
    }
}

void SctpSession::on_remoteup(struct socket* sock, void* arg, int flgs)
{
    RTC_LOG(LS_INFO) << "sctp remote on_remoteup";
    SctpSession* p = (SctpSession*)arg;
    if (!p) {
        RTC_LOG(LS_WARNING) << "on_remoteup failed, nullptr";
    }
 
    if(nullptr == p->_remote_socket)
    {
        p->_remote_socket = usrsctp_accept(sock, NULL, NULL);
    } else {
        RTC_LOG(LS_WARNING) << "_remote_socket is nullptr";
        return;
    }
    p->set_state(kConnected);
}

void SctpSession::handle_data_received(const char* data, size_t len) {
    // 控制信息发给usrsctp，数据信息的话发信号出去，发送时需要修改连接标签！
    
    if (!_local_socket) return;

    // 检查基本包长度
    const size_t MIN_PACKET_SIZE = 12; // 公共头长度
    if (!_local_socket || len < MIN_PACKET_SIZE) {
        RTC_LOG(LS_WARNING) << "Invalid SCTP packet size: " << len;
        return;
    }

    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data);
    
    // 解析公共头 (12字节)
    uint16_t source_port = ntohs(*reinterpret_cast<const uint16_t*>(buffer));
    uint16_t dest_port = ntohs(*reinterpret_cast<const uint16_t*>(buffer + 2));
    uint32_t verification_tag = ntohl(*reinterpret_cast<const uint32_t*>(buffer + 4));
    uint32_t checksum = ntohl(*reinterpret_cast<const uint32_t*>(buffer + 8));

    // 解析第一个chunk (从偏移量12开始)
    size_t chunk_offset = 12;
    if (len < chunk_offset + 4) {
        RTC_LOG(LS_WARNING) << "Packet too short for chunk header";
        return;
    }
    
    uint8_t chunk_type = buffer[chunk_offset];
    uint8_t chunk_flags = buffer[chunk_offset + 1];
    uint16_t chunk_length = ntohs(*reinterpret_cast<const uint16_t*>(buffer + chunk_offset + 2));
    
    const size_t CHUNK_TYPE_POS = 0;         // Type位置
    const size_t CHUNK_FLAGS_POS = 1;         // Flags位置
    const size_t CHUNK_LENGTH_POS = 2;        // Length位置 (2字节)
    const size_t TSN_POS = 4;                 // TSN位置
    const size_t STREAM_ID_POS = 8;           // Stream ID位置
    const size_t STREAM_SEQ_POS = 10;         // Stream Sequence位置
    const size_t PPID_POS = 12;               // Payload Protocol位置
    const size_t USER_DATA_POS = 16;          // 用户数据开始位置
    
    usrsctp_conninput(this, buffer, len, 0);
    if (chunk_type != 0) { 
        // 控制信息不用考虑转发问题
        RTC_LOG(LS_INFO) << "收到控制信息，类型：" << chunk_type;
        return;
    }
    
    // 到这说明是 sctp 数据信息，可能是 datachannel 控制信息或数据信息
    // 检查 DATA 第一个 chunk 最小长度（包括头部）
    const size_t MIN_DATA_CHUNK_SIZE = 16; // DATA chunk最小长度
    if (chunk_length < MIN_DATA_CHUNK_SIZE) {
        RTC_LOG(LS_WARNING) << "Invalid DATA chunk length: " << chunk_length 
                            << " < min size " << MIN_DATA_CHUNK_SIZE;
        return;
    }
    
    // 检查包长度是否足够
    size_t chunk_end = chunk_offset + chunk_length;
    if (chunk_end > len) {
        RTC_LOG(LS_WARNING) << "DATA chunk extends beyond packet, chunk_end: " 
                            << chunk_end << ", len: " << len;
        return;
    }
    
    // 解析DATA chunk固定头（16字节）
    const uint8_t* data_chunk = buffer + chunk_offset;
    
    // 提取各字段（根据图示精确位置）
    uint32_t tsn = ntohl(*reinterpret_cast<const uint32_t*>(data_chunk + TSN_POS));
    uint16_t stream_id = ntohs(*reinterpret_cast<const uint16_t*>(data_chunk + STREAM_ID_POS));
    uint16_t stream_seq = ntohs(*reinterpret_cast<const uint16_t*>(data_chunk + STREAM_SEQ_POS));
    uint32_t ppid = ntohl(*reinterpret_cast<const uint32_t*>(data_chunk + PPID_POS));
    
    // 解析标志位（根据图示）
    // U: 无序标志 (位0)
    // B: 起始分片 (位1)
    // E: 结束分片 (位2)
    bool unordered = (chunk_flags & 0x04) != 0;      // U位
    bool beginning_fragment = (chunk_flags & 0x02) != 0; // B位
    bool ending_fragment = (chunk_flags & 0x01) != 0;   // E位
    
    // webrtc几乎不使用多trunk的数据
    RTC_LOG(LS_INFO) << "第一个chunk:"
                     << "\n  类型: " << static_cast<int>(chunk_type)
                     << "\n  标志: " << static_cast<int>(chunk_flags)
                     << "\n  长度: " << chunk_length
                     << "\n  TSN: " << tsn;

    // 提取用户数据
    const size_t header_size = USER_DATA_POS;
    const uint8_t* payload_ptr = data_chunk + USER_DATA_POS;
    size_t payload_length = chunk_length - header_size;
    
    _processor.ProcessPayload(payload_ptr, payload_length, stream_id, ppid);
}

bool SctpSession::handle_output(void* buffer, size_t length) {
    on_send_to_local_(buffer, length);

    // 输出包结构信息（跳过数据段）
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    RTC_LOG(INFO) << "SCTP包结构:\n" 
                        << format_sctp_packet(data, length);

    return true;
}

int SctpSession::on_send_data(struct socket* sock, uint32_t sb_free, void* ulp_info) {
    return 1;
}

int SctpSession::on_sctp_event(struct socket *sock, union sctp_sockstore addr, void *data, size_t datalen,
                               struct sctp_rcvinfo rcv, int flags, void *user_data)
{
    SctpSession *self = static_cast<SctpSession *>(user_data);
    if (!self) {
        return 0;
    }

    return 0;
}

void SctpSession::SendDatachannelControlResponse(const std::vector<uint8_t>& response, uint16_t stream_id, uint32_t ppid) {
    // 使用 usrsctp 发送响应
    struct sctp_sndinfo sndinfo = {0};
    sndinfo.snd_sid = htonl(stream_id);
	sndinfo.snd_flags = SCTP_EOR;
	sndinfo.snd_ppid = htonl(ppid);
	sndinfo.snd_context = 0;
	sndinfo.snd_assoc_id = 0;

    int ret = usrsctp_sendv(_remote_socket, response.data(), response.size(), NULL, 0, (void *)&sndinfo, (socklen_t)sizeof(sndinfo), SCTP_SENDV_SNDINFO, 0);
    if(0 > ret) {
        RTC_LOG(LS_WARNING) << "usrsctp_sendv failed " << ret;
    }
}

void SctpSession::ForwardApplicationData(const uint8_t* data, size_t len) {
    // 发信号，转发给目标客户端

    // 原路返回信息，测试用
    // send_data((const char*)data, len);

    on_sctp_data_received_((void *)data, len);
    RTC_LOG(INFO) << "ForwardApplicationData was called";
}

} // namespace xrtc