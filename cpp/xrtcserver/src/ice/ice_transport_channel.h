#pragma once

#include <vector>
#include <string>

#include <base/event_loop.h>
#include <rtc_base/third_party/sigslot/sigslot.h>

#include "ice/ice_def.h"
#include "ice/port_allocator.h"
#include "ice/ice_credentials.h"
#include "ice/candidate.h"
#include "ice/stun.h"
#include "ice/ice_controller.h"

namespace xrtc {

class UDPPort;

class IceTransportChannel : public sigslot::has_slots<> {

public:
    IceTransportChannel(EventLoop* el, PortAllocator* allocator, const std::string& transport_name, IceCandidateComponent component);
    virtual ~IceTransportChannel();

    std::string& transport_name() {return _transport_name;}
    IceCandidateComponent component() {return _component;}

    void set_ice_params(const IceParameters& ice_params);
    void set_remote_ice_params(const IceParameters& ice_params);
    void gathering_candidate();

    std::string to_string();

    sigslot::signal2<IceTransportChannel*, const std::vector<Candidate>&> signal_candidate_allocate_done;
    sigslot::signal1<IceTransportChannel*> signal_receiving_state;
    sigslot::signal1<IceTransportChannel*> signal_writable_state;
    sigslot::signal4<IceTransportChannel*, const char*, size_t, int64_t> signal_read_packet;

private:
    void _on_unknown_address(UDPPort *port, const rtc::SocketAddress &addr, StunMessage *msg, const std::string &remote_ufrag);
    void _add_connection(IceConnection* conn);
    void _sort_connections_and_update_state();
    void _maybe_start_pinging();
    void _on_check_and_ping();
    void _on_connection_state_change(IceConnection* conn);
    void _on_connection_destroyed(IceConnection* conn);
    void _on_read_packet(IceConnection* conn, const char* buf, size_t len, int64_t ts);

    void _ping_connection(IceConnection* conn);
    void _maybe_switch_selected_connection(IceConnection *conn);
    void _switch_selected_connection(IceConnection *conn);
    void _update_connection_states();
    void _update_state();
    void _set_writable(bool writable);
    void _set_receiving(bool receiving);

    friend void ice_ping_cb(EventLoop* el, TimerWatcher* w, void* data);

private:
    EventLoop* _el;
    std::string _transport_name;        // 存媒体
    IceCandidateComponent _component;   // rtp还是rtcp
    PortAllocator* _allocator;
    IceParameters _ice_params;
    IceParameters _remote_ice_params;
    std::vector<Candidate> _local_candidates;
    std::unique_ptr<IceController> _ice_controller;
    bool _start_pinging = false;
    TimerWatcher* _ping_watcher = nullptr;
    int _cur_ping_interval = WEAK_PING_INTERVAL;
    int64_t _last_ping_sent_ms = 0;
    IceConnection* _selected_connection = nullptr;
    bool _receiving = false;
    bool _writable = false;
};















    
}