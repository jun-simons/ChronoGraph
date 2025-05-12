#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/Graph.h>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>


namespace chronograph {
namespace graph {
namespace algorithms {

bool isReachable(const Graph& g,
                 const std::string& start,
                 const std::string& target)
{
    // Maps node -> list of outgoing edge IDs
    const auto& outEdges = g.getOutgoing();
    // Maps edge ID -> Edge struct (with .to, .from)
    const auto& edges = g.getEdges();

    // Trivial self‐case: only if the node exists
    if (start == target) {
        return outEdges.find(start) != outEdges.end();
    }

    // Can't start if the node isn't known
    auto itStart = outEdges.find(start);
    if (itStart == outEdges.end()) {
        return false;
    }

    // Standard BFS on nodes
    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    visited.insert(start);
    q.push(start);

    while (!q.empty()) {
        const auto u = q.front(); q.pop();

        // For each outgoing edge ID from u...
        auto oit = outEdges.find(u);
        if (oit == outEdges.end()) continue;

        for (const auto& eid : oit->second) {
            // Find the edge record
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;

            const std::string& v = eit->second.to;
            // if found, its reachable
            if (v == target) return true;
            // Otherwise enqueue if unseen
            if (!visited.count(v)) {
                visited.insert(v);
                q.push(v);
            }
        }
    }

    return false;
}

std::vector<std::string> shortestPath(const Graph& g,
                                      const std::string& start,
                                      const std::string& target)
{
    const auto& outEdges = g.getOutgoing();
    const auto& edges    = g.getEdges();

    // Special case: path from a node to itself
    if (start == target) {
        // if the node exists, return path to just itself
        if (outEdges.find(start) != outEdges.end()) {
            return { start };
        } else {
            return {};
        }
    }

    // Ensure start and target exist
    if (outEdges.find(start) == outEdges.end() ||
        outEdges.find(target) == outEdges.end()) {
        return {};
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;
    // Map each visited node to its predecessor in the BFS tree
    std::unordered_map<std::string, std::string> prev;

    visited.insert(start);
    q.push(start);

    bool found = false;
    while (!q.empty() && !found) {
        auto u = q.front(); q.pop();

        auto oit = outEdges.find(u);
        if (oit == outEdges.end()) continue;

        for (const auto& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const std::string& v = eit->second.to;
            if (!visited.count(v)) {
                visited.insert(v);
                prev[v] = u;
                if (v == target) {
                    found = true;
                    break;
                }
                q.push(v);
            }
        }
    }

    if (!found) {
        return {}; // no path
    }

    // Reconstruct path from target back to start
    std::vector<std::string> path;
    for (std::string cur = target; ; ) {
        path.push_back(cur);
        if (cur == start) break;
        cur = prev[cur];
    }
    std::reverse(path.begin(), path.end());
    return path;
}


}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
