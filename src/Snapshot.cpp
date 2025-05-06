// src/Snapshot.cpp
#include "chronograph/Snapshot.h"

namespace chronograph {

Snapshot::Snapshot(const Graph& graph, std::int64_t timestamp) {
    // TODO: replay graph.getEventLog() up to `timestamp` into nodes_ and edges_
}

const std::unordered_map<std::string, Node>& Snapshot::getNodes() const {
    return nodes_;
}

const std::unordered_map<std::string, Edge>& Snapshot::getEdges() const {
    return edges_;
}

}  // namespace chronograph