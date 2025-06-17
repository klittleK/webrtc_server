#pragma once

namespace xrtc {

enum class IceCandidatePairState {
    WAITING,    // 连通性检查没开始
    IN_PROGRESS,    // 检查进行中
    SUCCESSED,  // 检查成功
    FAILED, // 检查失败
};

}