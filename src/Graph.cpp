// src/Graph.cpp
#include "chronograph/Graph.h"
#include "chronograph/Event.h"
#include "chronograph/Node.h"
#include "chronograph/Edge.h"

#include <random>
#include <sstream>

namespace chronograph {

//––– Helper to make a quick unique ID for each Event –––
// TODO: replace this with alternative UUID library
namespace {
    std::string generateEventId() {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<uint64_t> dist;
        std::ostringstream ss;
        ss << std::hex << dist(rng);
        return ss.str();
    }
} // anonymous

void Graph::addEvent(const Event& event) {
    eventLog_.push_back(event);
}

void Graph::addNode(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp) {
    // Append event
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::ADD_NODE;
    e.entityId = id;
    e.payload = attrs;
    addEvent(e);
}

void Graph::delNode(const std::string& id, std::int64_t timestamp) {
    // TODO: construct a DEL_NODE event and append
}

void Graph::addEdge(const std::string& id,
                    const std::string& from,
                    const std::string& to,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp) {
    // TODO: construct an ADD_EDGE event and append
}

void Graph::delEdge(const std::string& id, std::int64_t timestamp) {
    // TODO: construct a DEL_EDGE event and append
}

void Graph::updateNode(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    // TODO: construct an UPDATE_NODE event and append
}

void Graph::updateEdge(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    // TODO: construct an UPDATE_EDGE event and append
}

const std::vector<Event>& Graph::getEventLog() const {
    return eventLog_;
}

}  // namespace chronograph
