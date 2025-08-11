#include "pc/datachannel_processor.h"

#include <rtc_base/logging.h>

namespace xrtc {

// 控制消息类型定义
constexpr uint8_t DATA_CHANNEL_OPEN = 0x03;
constexpr uint8_t DATA_CHANNEL_ACK = 0x02;

void DataChannelProcessor::ProcessPayload(const uint8_t* payload, size_t len, uint16_t stream_id, uint32_t ppid) {
    if (len == 0) return;

    if (len > 0) {
        const size_t MAX_PRINT_BYTES = 64; // 最多打印的字节数
        size_t bytes_to_print = std::min(len, MAX_PRINT_BYTES);
        
        std::string hex_string = "  Payload (hex): ";
        std::string ascii_string = "  Payload (ASCII): ";
        
        for (size_t i = 0; i < bytes_to_print; i++) {
            // HEX格式 (每字节两个字符)
            char hex_byte[4];
            snprintf(hex_byte, sizeof(hex_byte), "%02X ", payload[i]);
            hex_string += hex_byte;
            
            // ASCII格式
            char c = static_cast<char>(payload[i]);
            ascii_string += (isprint(c) ? c : '.');
            
            // 每8字节换行
            if ((i + 1) % 8 == 0) {
                hex_string += "\n                ";
                ascii_string += "\n                  ";
            }
        }
        
        RTC_LOG(LS_INFO) << hex_string;
        RTC_LOG(LS_INFO) << ascii_string;
        
        if (len > MAX_PRINT_BYTES) {
            RTC_LOG(LS_INFO) << "  ... (truncated " << (len - MAX_PRINT_BYTES) << " bytes)";
        }
    } else {
        RTC_LOG(LS_INFO) << "  No payload data";
    }
    
    // 检查是否是控制消息（第一个字节为消息类型）
    uint8_t message_type = payload[0];
    
    if (message_type == DATA_CHANNEL_OPEN || 
        message_type == DATA_CHANNEL_ACK) {
        HandleControlMessage(payload, len, stream_id, ppid);
    } else {
        HandleDataMessage(payload, len);
    }
}

void DataChannelProcessor::HandleControlMessage(const uint8_t* data, size_t len, uint16_t stream_id, uint32_t ppid) {
    uint8_t message_type = data[0];
    
    switch (message_type) {
        case DATA_CHANNEL_OPEN:
            // 解析 OPEN 消息
            if (len >= 12) {
                uint16_t label_len = (data[8] << 8) | data[9];
                std::string label(reinterpret_cast<const char*>(data + 12), label_len);
                
                // 发送 ACK 响应
                if (control_response_callback_) {
                    std::vector<uint8_t> ack = {DATA_CHANNEL_ACK};
                    control_response_callback_(ack, stream_id, ppid);
                }
            }
            break;
            
        case DATA_CHANNEL_ACK:
            // 不需要响应 ACK
            break;
    }
}

void DataChannelProcessor::HandleDataMessage(const uint8_t* data, size_t len) {
    // 直接转发应用数据
    if (data_forward_callback_) {
        data_forward_callback_(data, len);
    }
}

void DataChannelProcessor::SetControlResponseCallback(ControlResponseCallback callback) {
    control_response_callback_ = callback;
}

void DataChannelProcessor::SetDataForwardCallback(DataForwardCallback callback) {
    data_forward_callback_ = callback;
}


}