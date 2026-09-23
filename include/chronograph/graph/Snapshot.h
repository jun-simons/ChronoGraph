// include/chronograph/Snapshot.h
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <chronograph/graph/GraphState.h>

namespace chronograph {

// forward declaration
class Graph;

class Snapshot {
public:
    // Build snapshot by replaying every event with timestamp <= `timestamp`
    Snapshot(const Graph& graph, std::int64_t timestamp);

    // Accessors for nodes and edges at this point in time
    const std::unordered_map<std::string, Node>& getNodes() const { return state_.nodes; }
    const std::unordered_map<std::string, Edge>& getEdges() const { return state_.edges; }

    // Access adjacency lists at this snapshot
    const std::unordered_map<std::string, std::vector<std::string>>&
        getOutgoing() const { return state_.outgoing; }
    const std::unordered_map<std::string, std::vector<std::string>>&
        getIncoming() const { return state_.incoming; }

private:
    GraphState state_;
};

}  // namespace chronograph