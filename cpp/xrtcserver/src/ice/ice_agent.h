#pragma once

#include <string>
#include <vector>

#include "ice/ice_def.h"
#include "base/event_loop.h"
#include "ice/ice_transport_channel.h"
#include "ice/port_allocator.h"

namespace xrtc {

class IceAgent : public sigslot::has_slots<> {

public:
    IceAgent(EventLoop* el, PortAllocator* allocator);
    ~IceAgent();
    
    bool create_channel(EventLoop* el, const std::string& transport_name, IceCandidateComponent component);
    IceTransportChannel* get_channel(const std::string& transport_name, IceCandidateComponent component);

    void set_ice_params(const std::string& transport_name, IceCandidateComponent component, const IceParameters& ice_params);
    void set_remote_ice_params(const std::string& transport_name, IceCandidateComponent component, const IceParameters& ice_params);
    void gathering_candidate();
    sigslot::signal4<IceAgent*, const std::string&, IceCandidateComponent, const std::vector<Candidate>&> signal_candidate_allocate_done;

private:
    std::vector<IceTransportChannel*>::iterator _get_channel(const std::string& transport_name, IceCandidateComponent component);
    void on_candidate_allocate_done(IceTransportChannel* channel, const std::vector<Candidate>& candidates); 

private:
    EventLoop* _el;
    std::vector<IceTransportChannel*> _channels;
    PortAllocator* _allocator;
};

}