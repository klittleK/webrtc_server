#pragma once

namespace xrtc {

enum class PeerConnectionState {
    k_new,
    k_connecting,
    k_connected,
    k_disconnected,
    k_failed,
    k_closed,
};


}