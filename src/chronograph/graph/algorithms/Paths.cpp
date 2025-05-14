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

bool isReachableAt(const Graph& g,
    const std::string& start,
    const std::string& target,
    std::int64_t timestamp)
{
    // Build snapshot at T
    Snapshot snap(g, timestamp);

    // Use the same BFS logic, but on the snapshot
    const auto& outEdges = snap.getOutgoing();
    const auto& edges    = snap.getEdges();

    if (start == target) {
        return outEdges.find(start) != outEdges.end();
    }
    auto itStart = outEdges.find(start);
    if (itStart == outEdges.end()) {
        return false;
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;
    visited.insert(start);
    q.push(start);

    while (!q.empty()) {
        const auto u = q.front(); q.pop();
        auto oit = outEdges.find(u);
        if (oit == outEdges.end()) continue;
        for (const auto& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const auto& v = eit->second.to;
            if (v == target) return true;
            if (!visited.count(v)) {
                visited.insert(v);
                q.push(v);
            }
        }
    }
    return false;
}

bool isTimeRespectingReachable(const Graph& g,
    const std::string& start,
    const std::string& target)
{
    // Quick checks
    const auto& nodes = g.getNodes();
    if (!nodes.count(start) || !nodes.count(target)) {
        return false;
    }
    if (start == target) {
        return true;
    }

    // Adjacency: node -> list of edge-IDs
    const auto& outEdges = g.getOutgoing();
    // Edge storage: edge-ID -> Edge
    const auto& edges = g.getEdges();

    // Keep track of the smallest timestamp at reached each node
    std::unordered_map<std::string, std::int64_t> bestTime;
    std::queue<std::pair<std::string, std::int64_t>> q;

    // Initialize
    bestTime[start] = std::numeric_limits<std::int64_t>::min();
    q.push({start, bestTime[start]});

    while (!q.empty()) {
        auto [u, lastTs] = q.front();
        q.pop();

        auto oit = outEdges.find(u);
        if (oit == outEdges.end()) continue;

        for (auto const& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const auto& edge = eit->second;
            std::int64_t ts  = edge.createdTimestamp;  // creation time of this edge

            // skip "back in time edges"
            // only allows nondecreasing paths
            if (ts < lastTs) continue;

            const auto& v = edge.to;
            if (v == target) {
                return true;
            }

            // if v is not visited, or found a strictly smaller arrival time
            auto bestIt = bestTime.find(v);
            if (bestIt==bestTime.end() || ts < bestIt->second) {
                bestTime[v] = ts;
                q.push({v, ts});
            }
        }
    }

    return false;
}

// Directed cycle detection via DFS + recursion stack
static bool dfsDetectCycle(
    const Graph& g,
    const std::string& u,
    std::unordered_map<std::string,int>& state  // 0=unseen,1=visiting,2=done
) {
    state[u] = 1;  // visiting

    const auto& outEdges = g.getOutgoing();
    const auto& edges    = g.getEdges();

    if (auto oit = outEdges.find(u); oit != outEdges.end()) {
        for (auto const& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const auto& v = eit->second.to;

            // if neighbor is in recursion stack -> cycle
            if (state[v] == 1) {
                return true;
            }
            // if unseen, recurse
            if (state[v] == 0) {
                if (dfsDetectCycle(g, v, state)) {
                    return true;
                }
            }
        }
    }

    state[u] = 2;  // done
    return false;
}

bool hasCycle(const Graph& g) {
    const auto& nodes = g.getNodes();
    // track visitation state for each node
    std::unordered_map<std::string,int> state;
    state.reserve(nodes.size());
    for (auto const& [nid, _] : nodes) {
        state[nid] = 0;
    }

    // run DFS from each unvisited node
    for (auto const& [nid, _] : nodes) {
        if (state[nid] == 0) {
            if (dfsDetectCycle(g, nid, state)) {
                return true;
            }
        }
    }
    return false;
}




}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
