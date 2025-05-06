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
    std::int64_t timestamp;  // milliseconds since epoch
    EventType type;
    std::string entityId;
    std::map<std::string, std::string> payload;  // arbitrary key/value
};

}  // namespace chronograph
