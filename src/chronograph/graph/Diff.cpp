// src/Diff.cpp
#include <chronograph/graph/Diff.h>
#include <chronograph/graph/GraphView.h>

namespace chronograph {

DiffResult diff(const GraphView& before, const GraphView& after) {
    const auto& N1 = before.getNodes();
    const auto& N2 = after.getNodes();
    const auto& E1 = before.getEdges();
    const auto& E2 = after.getEdges();

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

}  // namespace chronograph
