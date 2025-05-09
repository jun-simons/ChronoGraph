// src/chronograph/graph/algorithms/Paths.cpp
#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/Graph.h>           // for Graph::getOutgoing()
#include <queue>
#include <unordered_set>

namespace chronograph {
namespace graph {
namespace algorithms {

bool isReachable(const Graph& g,
                 const std::string& start,
                 const std::string& target)
{
    // Grab the adjacency map
    const auto& adj = g.getOutgoing();

    // If start == target, consider reachable iff start exists
    if (start == target) {
        return adj.find(start) != adj.end();
    }

    // If there's no entry for 'start', we can't even begin
    auto itStart = adj.find(start);
    if (itStart == adj.end()) {
        return false;
    }

    // BFS
    std::unordered_set<std::string> visited;
    std::queue<std::string> q;
    if (adj.find(start) == adj.end()) return false;
    visited.insert(start);
    q.push(start);

    while (!q.empty()) {
        auto u = q.front(); q.pop();
        // Get its neighbors safely
        auto it = adj.find(u);
        // should only happen if edges reference missing nodes
        if (it == adj.end()) continue;  
        for (const auto& v : it->second) {
            if (v == target) return true;
            if (!visited.count(v)) {
                visited.insert(v);
                q.push(v);
            }
        }
    }
    return false;
}

}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
