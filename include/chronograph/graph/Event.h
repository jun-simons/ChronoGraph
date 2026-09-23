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

inline bool operator==(const Event& a, const Event& b) {
    return a.id == b.id && a.timestamp == b.timestamp && a.type == b.type &&
           a.entityId == b.entityId && a.payload == b.payload &&
           a.from == b.from && a.to == b.to;
}
inline bool operator!=(const Event& a, const Event& b) { return !(a == b); }

}  // namespace chronograph
