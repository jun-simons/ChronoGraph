// src/Graph.cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/Diff.h>

#include <random>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>

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
    state_.apply(event);
    maxTimestamp_ = std::max(maxTimestamp_, event.timestamp);
    maybeCreateCheckpoint();
}
void Graph::record(Event e) {
    e.id = generateEventId();
    addEvent(e);
}
const std::vector<Event>& Graph::getEventLog() const { 
    return eventLog_; 
}
const std::vector<Graph::Checkpoint>& Graph::getCheckpoints() const { 
    return checkpoints_; 
}
void Graph::maybeCreateCheckpoint() {
    if (eventLog_.size() % kCheckpointInterval == 0) {
        checkpoints_.push_back({maxTimestamp_, eventLog_.size(), state_});
    }
}

namespace {
    [[noreturn]] void reject(const std::string& what, const std::string& id,
                             const std::string& problem) {
        throw std::invalid_argument(what + " '" + id + "' " + problem);
    }
} // anonymous

void Graph::addNode(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp) {
    if (state_.nodes.count(id)) reject("Node", id, "already exists");

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::ADD_NODE;
    e.entityId = id;
    e.payload = attrs;
    record(std::move(e));
}

void Graph::delNode(const std::string& id, std::int64_t timestamp) {
    if (!state_.nodes.count(id)) reject("Node", id, "does not exist");

    // Record the cascading edge deletions first, so replaying the log never
    // sees an edge event for a node that is already gone
    std::vector<std::string> incident = state_.outgoing.at(id);
    const auto& in = state_.incoming.at(id);
    incident.insert(incident.end(), in.begin(), in.end());
    for (const auto& eid : incident) {
        if (state_.edges.count(eid)) {  // self-loops appear in both lists
            delEdge(eid, timestamp);
        }
    }

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::DEL_NODE;
    e.entityId = id;
    record(std::move(e));
}

void Graph::addEdge(const std::string& id,
                    const std::string& from,
                    const std::string& to,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp) {
    if (state_.edges.count(id)) reject("Edge", id, "already exists");
    if (!state_.nodes.count(from)) reject("Edge", id, "source node '" + from + "' does not exist");
    if (!state_.nodes.count(to)) reject("Edge", id, "target node '" + to + "' does not exist");

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::ADD_EDGE;
    e.entityId  = id;
    e.payload  = attrs;
    e.from = from;
    e.to = to;
    record(std::move(e));
}

void Graph::delEdge(const std::string& id, std::int64_t timestamp) {
    auto it = state_.edges.find(id);
    if (it == state_.edges.end()) reject("Edge", id, "does not exist");

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::DEL_EDGE;
    e.entityId = id;
    e.from = it->second.from;
    e.to = it->second.to;
    // no payload for deletions
    record(std::move(e));
}

void Graph::updateNode(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    if (!state_.nodes.count(id)) reject("Node", id, "does not exist");

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::UPDATE_NODE;
    e.entityId = id;
    e.payload = attrs;
    record(std::move(e));
}

void Graph::updateEdge(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    if (!state_.edges.count(id)) reject("Edge", id, "does not exist");

    Event e;
    e.timestamp = timestamp;
    e.type = EventType::UPDATE_EDGE;
    e.entityId = id;
    e.payload = attrs;
    record(std::move(e));
}

// Diff between the states at t1 and t2
DiffResult Graph::diff(std::int64_t t1, std::int64_t t2) const {
    return chronograph::diff(Snapshot(*this, t1), Snapshot(*this, t2));
}

void Graph::clearGraph() {
    eventLog_.clear();
    state_.clear();
    checkpoints_.clear();
    maxTimestamp_ = std::numeric_limits<std::int64_t>::min();
}

}  // namespace chronograph
