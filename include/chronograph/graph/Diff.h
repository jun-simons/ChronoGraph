// include/chronograph/graph/Diff.h
#pragma once

#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <string>
#include <utility>
#include <vector>

namespace chronograph {

class GraphView;

/// Changes that turn one graph state into another
struct DiffResult {
    // Nodes
    std::vector<Node> nodesAdded;
    std::vector<std::string> nodesRemoved;
    std::vector<std::pair<Node,Node>> nodesUpdated;   // {before, after}

    // Edges
    std::vector<Edge> edgesAdded;
    std::vector<std::string> edgesRemoved;
    std::vector<std::pair<Edge,Edge>> edgesUpdated;   // {before, after}

    bool empty() const {
        return nodesAdded.empty() && nodesRemoved.empty() && nodesUpdated.empty() &&
               edgesAdded.empty() && edgesRemoved.empty() && edgesUpdated.empty();
    }
};

/**
 * Compare two graph states:
 *   • added   = in `after` but not in `before`
 *   • removed = in `before` but not in `after`
 *   • updated = in both, but attributes (or, for edges, endpoints) differ
 * Works on any GraphView: Graphs, Snapshots, or a mix of the two.
 */
DiffResult diff(const GraphView& before, const GraphView& after);

}  // namespace chronograph
