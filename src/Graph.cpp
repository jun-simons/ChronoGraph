// src/Graph.cpp
#include "chronograph/Graph.h"
#include "chronograph/Event.h"
#include "chronograph/Node.h"
#include "chronograph/Edge.h"

#include <random>
#include <sstream>
#include <algorithm>

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

    // Update graph state
    nodes_[id] = Node{id, attrs};

    // TODO: properly update adjacency lists
    outgoing_.emplace(id, std::vector<std::string>{});
    incoming_.emplace(id, std::vector<std::string>{});
}

void Graph::delNode(const std::string& id, std::int64_t timestamp) {
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::DEL_NODE;
    e.entityId = id;
    addEvent(e);

    // Delete all outgoing edges
    if (auto oit = outgoing_.find(id); oit != outgoing_.end()) {
        auto edgesToDel = oit->second;  // copy list
        for (const auto& eid : edgesToDel) {
            // ensure we only delete once
            if (edges_.count(eid)) {
                delEdge(eid, timestamp);
            }
        }
    }

    // Delete all incoming edges
    if (auto iit = incoming_.find(id); iit != incoming_.end()) {
        auto edgesToDel = iit->second;  // copy list
        for (const auto& eid : edgesToDel) {
            if (edges_.count(eid)) {
                delEdge(eid, timestamp);
            }
        }
    }

    // Erase the node itself and its adjacency lists
    nodes_.erase(id);
    outgoing_.erase(id);
    incoming_.erase(id);
}

void Graph::addEdge(const std::string& id,
                    const std::string& from,
                    const std::string& to,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp) {
    // Append event
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::ADD_EDGE;
    e.entityId  = id;
    e.payload  = attrs;
    addEvent(e);
    // Store the edge
    edges_[id] = Edge{id, from, to, attrs};
    // Update adjacency
    outgoing_[from].push_back(id);
    incoming_[to].push_back(id);
}

void Graph::delEdge(const std::string& id, std::int64_t timestamp) {
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::DEL_EDGE;
    e.entityId = id;
    // no payload for deletions
    addEvent(e);

    auto it = edges_.find(id);
    if (it == edges_.end()) return;

    // extract endpoints before erasing
    const std::string from = it->second.from;
    const std::string to   = it->second.to;

    edges_.erase(it);

    // remove from outgoing[from]
    auto& outList = outgoing_[from];
    outList.erase(std::remove(outList.begin(), outList.end(), id),
                  outList.end());

    // remove from incoming[to]
    auto& inList  = incoming_[to];
    inList.erase(std::remove(inList.begin(), inList.end(), id),
                 inList.end());
}

void Graph::updateNode(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::UPDATE_NODE;
    e.entityId = id;
    e.payload = attrs;
    addEvent(e);
    // Merge into live node, if it exists
    auto it = nodes_.find(id);
    if (it != nodes_.end()) {
        auto& nodeAttrs = it->second.attributes;
        for (const auto& [k,v] : attrs) {
            nodeAttrs[k] = v;
        }
    }
}

void Graph::updateEdge(const std::string& id,
                       const std::map<std::string, std::string>& attrs,
                       std::int64_t timestamp) {
    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::UPDATE_EDGE;
    e.entityId = id;
    e.payload = attrs;
    addEvent(e);
    // Merge into live edge, if it exists
    auto it = edges_.find(id);
    if (it != edges_.end()) {
        auto& edgeAttrs = it->second.attributes;
        for (const auto& [k,v] : attrs) {
            edgeAttrs[k] = v;
        }
    }
}

// Getters

const std::vector<Event>&
Graph::getEventLog() const {
    return eventLog_;
}

const std::unordered_map<std::string, Node>&
Graph::getNodes() const {
    return nodes_;
}

const std::unordered_map<std::string, Edge>&
Graph::getEdges() const {
    return edges_;
}

const std::unordered_map<std::string, std::vector<std::string>>&
Graph::getOutgoing() const {
    return outgoing_;
}

const std::unordered_map<std::string, std::vector<std::string>>&
Graph::getIncoming() const {
    return incoming_;
}

}  // namespace chronograph
