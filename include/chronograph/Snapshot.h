// include/chronograph/Snapshot.h
#pragma once

#include "Graph.h"
#include <unordered_map>

namespace chronograph {

class Snapshot {
public:
    // Build snapshot by replaying events up to and including `timestamp`
    Snapshot(const Graph& graph, std::int64_t timestamp);

    // Accessors for nodes and edges at this point in time
    const std::unordered_map<std::string, Node>& getNodes() const;
    const std::unordered_map<std::string, Edge>& getEdges() const;

private:
    std::unordered_map<std::string, Node> nodes_;
    std::unordered_map<std::string, Edge> edges_;
    std::unordered_map<std::string, std::vector<std::string>> outgoing_;
    std::unordered_map<std::string, std::vector<std::string>> incoming_;
};

}  // namespace chronograph