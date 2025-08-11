#pragma once

#include <functional>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace xrtc {
    class DataChannelProcessor {
        public:
            // 处理传入的 payload
            void ProcessPayload(const uint8_t* payload, size_t len, uint16_t stream_id, uint32_t ppid);
            
            // 设置回调函数
            using ControlResponseCallback = std::function<void(const std::vector<uint8_t>& response, uint16_t stream_id, uint32_t ppid)>;
            using DataForwardCallback = std::function<void(const uint8_t* data, size_t len)>;
            
            void SetControlResponseCallback(ControlResponseCallback callback);
            void SetDataForwardCallback(DataForwardCallback callback);
        
        private:
            void HandleControlMessage(const uint8_t* data, size_t len, uint16_t stream_id, uint32_t ppid);
            void HandleDataMessage(const uint8_t* data, size_t len);
            
            ControlResponseCallback control_response_callback_;
            DataForwardCallback data_forward_callback_;
    };

}