// include/chronograph/Node.h
#pragma once

#include <string>
#include <map>

namespace chronograph {

struct Node {
    std::string id;
    std::map<std::string, std::string> attributes;
};

}  // namespace chronograph