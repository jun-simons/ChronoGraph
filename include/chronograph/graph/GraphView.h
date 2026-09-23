// include/chronograph/graph/GraphView.h
#pragma once

#include <chronograph/graph/GraphState.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace chronograph {

/// Read-only access to a materialized graph.
// * Base of both Graph (the live state) and Snapshot (the state at a point in
//   time), so algorithms, diffs and exporters written against GraphView work
//   on either.
// * Not polymorphic and not constructible on its own: it only shares the
//   accessors over the GraphState its subclasses maintain.
class GraphView {
public:
    const std::unordered_map<std::string, Node>& getNodes() const { return state_.nodes; }
    const std::unordered_map<std::string, Edge>& getEdges() const { return state_.edges; }

    // Adjacency: node ID -> IDs of its outgoing / incoming edges
    const std::unordered_map<std::string, std::vector<std::string>>&
        getOutgoing() const { return state_.outgoing; }
    const std::unordered_map<std::string, std::vector<std::string>>&
        getIncoming() const { return state_.incoming; }

    bool hasNode(const std::string& id) const { return state_.nodes.count(id) > 0; }
    bool hasEdge(const std::string& id) const { return state_.edges.count(id) > 0; }

protected:
    GraphView() = default;
    GraphView(const GraphView&) = default;
    GraphView(GraphView&&) = default;
    GraphView& operator=(const GraphView&) = default;
    GraphView& operator=(GraphView&&) = default;
    ~GraphView() = default;

    GraphState state_;
};

}  // namespace chronograph
