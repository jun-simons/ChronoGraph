#include <chronograph/graph/algorithms/Connectivity.h>
#include <chronograph/graph/Graph.h>
#include <queue>
#include <unordered_set>
#include <vector>

namespace chronograph {
namespace graph {
namespace algorithms {

std::vector<std::vector<std::string>>
weaklyConnectedComponents(const Graph& g) {
    // Grab adjacency in both directions
    const auto& out = g.getOutgoing();
    const auto& in  = g.getIncoming();
    const auto& edges = g.getEdges();
    const auto& nodes = g.getNodes();

    std::unordered_set<std::string> visited;
    std::vector<std::vector<std::string>> components;
    components.reserve(nodes.size());

    // For every node in the graph:
    for (const auto& [nid, node] : nodes) {
        if (visited.count(nid)) continue;

        // Start a BFS from nid over undirected neighbors
        std::vector<std::string> comp;
        std::queue<std::string> q;
        visited.insert(nid);
        q.push(nid);

        while (!q.empty()) {
            auto u = q.front(); q.pop();
            comp.push_back(u);

            // Outgoing edges → neighbors
            if (auto oit = out.find(u); oit != out.end()) {
                for (auto& eid : oit->second) {
                    if (auto eit = edges.find(eid); eit != edges.end()) {
                        const auto& v = eit->second.to;
                        if (!visited.count(v)) {
                            visited.insert(v);
                            q.push(v);
                        }
                    }
                }
            }

            // Incoming edges → neighbors
            if (auto iit = in.find(u); iit != in.end()) {
                for (auto& eid : iit->second) {
                    if (auto eit = edges.find(eid); eit != edges.end()) {
                        const auto& v = eit->second.from;
                        if (!visited.count(v)) {
                            visited.insert(v);
                            q.push(v);
                        }
                    }
                }
            }
        }

        components.push_back(std::move(comp));
    }

    return components;
}

}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
