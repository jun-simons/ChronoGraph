// include/chronograph/Edge.h
#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace chronograph {

struct Edge {
    std::string id;
    std::string from;  // source node ID
    std::string to;    // target node ID
    std::map<std::string, std::string> attributes;
    std::int64_t createdTimestamp = 0;  // timestamp of the ADD_EDGE event
};

}  // namespace chronograph