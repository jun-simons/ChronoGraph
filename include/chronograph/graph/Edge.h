// include/chronograph/Edge.h
#pragma once

#include <string>
#include <map>

namespace chronograph {

struct Edge {
    std::string id;
    std::string from;  // source node ID
    std::string to;    // target node ID
    std::map<std::string, std::string> attributes;
};

}  // namespace chronograph