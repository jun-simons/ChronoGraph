// include/chronograph/graph/GraphState.h
#pragma once

#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace chronograph {

/// Materialized graph state: nodes, edges and adjacency (node -> edge IDs).
// * `apply()` is the single definition of how an Event changes the graph;
//   Graph, Snapshot and checkpoints all build their state through it.
// * `apply()` does no validation: it is used to replay history, which may come
//   from merged branches. Validation lives in Graph's mutators.
struct GraphState {
    std::unordered_map<std::string, Node> nodes;
    std::unordered_map<std::string, Edge> edges;
    std::unordered_map<std::string, std::vector<std::string>> outgoing;
    std::unordered_map<std::string, std::vector<std::string>> incoming;

    void apply(const Event& e);
    void clear();

private:
    void removeEdge(const std::string& id);
};

}  // namespace chronograph
