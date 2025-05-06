// include/versioned_graph/Node.h
#pragma once

#include <string>
#include <map>

namespace versioned_graph {

struct Node {
    std::string id;
    std::map<std::string, std::string> attributes;
};

}  // namespace versioned_graph