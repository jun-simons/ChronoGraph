// include/chronograph/Node.h
#pragma once

#include <string>
#include <map>

namespace chronograph {

struct Node {
    std::string id;
    std::map<std::string, std::string> attributes;
};

inline bool operator==(const Node& a, const Node& b) {
    return a.id == b.id && a.attributes == b.attributes;
}
inline bool operator!=(const Node& a, const Node& b) { return !(a == b); }

}  // namespace chronograph