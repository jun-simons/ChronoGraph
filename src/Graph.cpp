// src/Graph.cpp
#include "chronograph/Graph.h"
#include "chronograph/Event.h"
#include "chronograph/Node.h"
#include "chronograph/Edge.h"
#include "chronograph/Snapshot.h"

#include <random>
#include <sstream>
#include <algorithm>
#include <utility>

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
    // after state update in each mutator, call:
    // maybeCreateCheckpoint(event);
}
const std::vector<Event>& Graph::getEventLog() const { 
    return eventLog_; 
}
const std::vector<Graph::Checkpoint>& Graph::getCheckpoints() const { 
    return checkpoints_; 
}
void Graph::maybeCreateCheckpoint(const Event& e) {
    if (eventLog_.size() % kCheckpointInterval == 0) {
        checkpoints_.push_back({
            /*timestamp=*/e.timestamp,
            /*eventIndex=*/eventLog_.size(),
            /*nodes=*/nodes_,
            /*edges=*/edges_,
            /*outgoing=*/outgoing_,
            /*incoming=*/incoming_
        });
    }
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
    maybeCreateCheckpoint(e);
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

    maybeCreateCheckpoint(e);
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
    e.from = from;
    e.to = to;
    addEvent(e);
    // Store the edge
    edges_[id] = Edge{id, from, to, attrs};
    // Update adjacency
    outgoing_[from].push_back(id);
    incoming_[to].push_back(id);

    maybeCreateCheckpoint(e);
}

void Graph::delEdge(const std::string& id, std::int64_t timestamp) {
    auto it = edges_.find(id);
    if (it == edges_.end()) return;

    // extract endpoints before erasing
    const std::string from = it->second.from;
    const std::string to   = it->second.to;

    Event e;
    e.id = generateEventId();
    e.timestamp = timestamp;
    e.type = EventType::DEL_EDGE;
    e.entityId = id;
    e.from = from;
    e.to = to;
    // no payload for deletions
    addEvent(e);

    edges_.erase(it);

    // remove from outgoing[from]
    auto& outList = outgoing_[from];
    outList.erase(std::remove(outList.begin(), outList.end(), id),
                  outList.end());

    // remove from incoming[to]
    auto& inList  = incoming_[to];
    inList.erase(std::remove(inList.begin(), inList.end(), id),
                 inList.end());

    maybeCreateCheckpoint(e);
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

    maybeCreateCheckpoint(e);
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

    maybeCreateCheckpoint(e);
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

// Graph Getters

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
