#pragma once

#include <string>

namespace xrtc {

struct IceParameters {
    IceParameters() = default;
    IceParameters(const std::string& ufrag, const std::string& pwd) : ice_ufrag(ufrag), ice_pwd(pwd) {}

    std::string ice_ufrag;
    std::string ice_pwd;
};

class IceCredentials
{

public:
    static IceParameters create_random_ice_credentials();

private:
    
};














}