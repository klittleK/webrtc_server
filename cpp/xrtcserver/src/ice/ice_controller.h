#pragma once

#include <set>

#include "ice/ice_connection.h"

namespace xrtc {

class IceTransportChannel;

struct PingResult {
    PingResult(const IceConnection* conn, int ping_interval) : 
        conn(conn), ping_interval(ping_interval) {}
    
    const IceConnection* conn = nullptr;
    int ping_interval = 0;
};

class IceController {

public:
    IceController(IceTransportChannel* ice_channel) : _ice_channel(ice_channel) {}
    ~IceController() = default;

    void add_connection(IceConnection* conn);
    const std::vector<IceConnection*> connections() { return _connections; }
    bool has_pingable_connection();
    PingResult select_connection_to_ping(int64_t last_ping_sent_ms);
    IceConnection* sort_and_switch_connection();
    void set_selected_connection(IceConnection* conn) { _selected_connection = conn; }
    void mark_connection_pinged(IceConnection* conn);
    void on_connection_destroyed(IceConnection* conn);

private:
    bool _weak() {
        return _selected_connection == nullptr || _selected_connection->weak();
    }

    bool _is_pingable(IceConnection* conn, int64_t now);
    const IceConnection* _find_next_pingable_connection(int64_t last_ping_sent_ms);
    bool _is_connection_past_ping_interval(const IceConnection* conn, int64_t now);
    int _get_connection_ping_interval(const IceConnection* conn, int64_t now);
    bool _more_pingable(IceConnection* conn1, IceConnection* conn2);

    int _compare_connections(IceConnection* a, IceConnection* b);
    bool _ready_to_send(IceConnection* conn);

private:
    IceTransportChannel* _ice_channel;
    IceConnection* _selected_connection = nullptr;
    std::vector<IceConnection*> _connections;
    std::set<IceConnection*> _unpinged_connections;
    std::set<IceConnection*> _pinged_connections;

};




}