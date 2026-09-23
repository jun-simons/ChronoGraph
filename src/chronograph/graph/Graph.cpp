// src/Graph.cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <chronograph/graph/Snapshot.h>

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

// DIFF 
/** 
     * Compute the “diff” from time t1 -> t2:
     *   • nodesAdded   = in snapshot(t2) but not in snapshot(t1)
     *   • nodesRemoved = in snapshot(t1) but not in snapshot(t2)
     *   • nodesUpdated = in both, but attributes differ
     * similarly for edges.
 */
Graph::DiffResult Graph::diff(std::int64_t t1, std::int64_t t2) const {
    // Build snapshots
    Snapshot s1(*this, t1);
    Snapshot s2(*this, t2);

    const auto& N1 = s1.getNodes();
    const auto& N2 = s2.getNodes();
    const auto& E1 = s1.getEdges();
    const auto& E2 = s2.getEdges();

    DiffResult result;

    // Nodes added & updated
    for (auto& [id2, node2] : N2) {
        auto it1 = N1.find(id2);
        if (it1 == N1.end()) {
            // brand-new node
            result.nodesAdded.push_back(node2);
        } else {
            // existed before -> check for attribute changes
            const Node& node1 = it1->second;
            if (node1.attributes != node2.attributes) {
                result.nodesUpdated.emplace_back(node1, node2);
            }
        }
    }

    // Nodes removed
    for (auto& [id1, node1] : N1) {
        if (N2.find(id1) == N2.end()) {
            result.nodesRemoved.push_back(id1);
        }
    }

    // Edges added & updated
    for (auto& [id2, edge2] : E2) {
        auto it1 = E1.find(id2);
        if (it1 == E1.end()) {
            result.edgesAdded.push_back(edge2);
        } else {
            const Edge& edge1 = it1->second;
            if (edge1.attributes != edge2.attributes ||
                edge1.from != edge2.from  ||
                edge1.to != edge2.to) {
                result.edgesUpdated.emplace_back(edge1, edge2);
            }
        }
    }

    // Edges removed
    for (auto& [id1, edge1] : E1) {
        if (E2.find(id1) == E2.end()) {
            result.edgesRemoved.push_back(id1);
        }
    }

    return result;
}

void Graph::clearGraph() {
    eventLog_.clear();
    state_.clear();
    checkpoints_.clear();
    maxTimestamp_ = std::numeric_limits<std::int64_t>::min();
}

// ---- Graph Getters ----

const std::unordered_map<std::string, Node>&
Graph::getNodes() const {
    return state_.nodes;
}

const std::unordered_map<std::string, Edge>&
Graph::getEdges() const {
    return state_.edges;
}

const std::unordered_map<std::string, std::vector<std::string>>&
Graph::getOutgoing() const {
    return state_.outgoing;
}

const std::unordered_map<std::string, std::vector<std::string>>&
Graph::getIncoming() const {
    return state_.incoming;
}

}  // namespace chronograph
