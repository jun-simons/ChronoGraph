// include/chronograph/Event.h
#pragma once

#include <string>
#include <cstdint>
#include <map>

namespace chronograph {

enum class EventType {
    ADD_NODE,
    DEL_NODE,
    ADD_EDGE,
    DEL_EDGE,
    UPDATE_NODE,
    UPDATE_EDGE
};

struct Event {
    std::string id;
    std::int64_t timestamp;
    EventType type;
    std::string entityId; // nodeId or edgeId
    std::map<std::string,std::string> payload; // attributes for ADD_/UPDATE_
    // for edge‐events, record endpoints
    std::string from;
    std::string to;
};

}  // namespace chronograph
