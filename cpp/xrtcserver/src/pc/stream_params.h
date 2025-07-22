#pragma once

#include <vector>
#include <string>

namespace xrtc {

struct SsrcGroup {
    SsrcGroup(const std::string& semantics, const std::vector<uint32_t> ssrcs);

    std::string semantics;
    std::vector<uint32_t> ssrcs;
};

struct StreamParams {
    bool has_ssrc(uint32_t ssrc);

    std::string id;
    std::vector<uint32_t> ssrcs;
    std::vector<SsrcGroup> ssrc_groups;
    std::string cname;
    std::string stream_id;
};

}