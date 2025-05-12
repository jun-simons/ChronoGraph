// include/chronograph/Snapshot.h
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include  <chronograph/graph/Node.h>
#include  <chronograph/graph/Edge.h>

namespace chronograph {

// forward declaration
class Graph;

class Snapshot {
public:
    // Build snapshot by replaying events up to and including `timestamp`
    Snapshot(const Graph& graph, std::int64_t timestamp);

    // Accessors for nodes and edges at this point in time
    const std::unordered_map<std::string, Node>& getNodes() const { return nodes_; }
    const std::unordered_map<std::string, Edge>& getEdges() const { return edges_; }

    // Access adjacency lists at this snapshot
    const std::unordered_map<std::string, std::vector<std::string>>&
        getOutgoing() const { return outgoing_; }
    const std::unordered_map<std::string, std::vector<std::string>>&
        getIncoming() const { return incoming_; }

private:
    std::unordered_map<std::string, Node> nodes_;
    std::unordered_map<std::string, Edge> edges_;
    std::unordered_map<std::string, std::vector<std::string>> outgoing_;
    std::unordered_map<std::string, std::vector<std::string>> incoming_;
};

}  // namespace chronograph