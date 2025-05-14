#include <chronograph/graph/algorithms/Connectivity.h>
#include <chronograph/graph/Graph.h>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

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

static void tarjanDFS(const Graph& g,
    const std::string& v,
    int& index,
    std::unordered_map<std::string,int>& indexes,
    std::unordered_map<std::string,int>& lowlinks,
    std::vector<std::string>& stack,
    std::unordered_set<std::string>& onStack,
    std::vector<std::vector<std::string>>& comps)
    {
    indexes[v] = index;
    lowlinks[v] = index;
    ++index;

    stack.push_back(v);
    onStack.insert(v);

    // For each neighbor w of v
    const auto& out = g.getOutgoing();
    const auto& edges = g.getEdges();
    auto oit = out.find(v);
    if (oit != out.end()) {
        for (const auto& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const auto& w = eit->second.to;

            if (!indexes.count(w)) {
            // not visited
                tarjanDFS(g, w, index, indexes, lowlinks, stack, onStack, comps);
                lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
            }
            else if (onStack.count(w)) {
                lowlinks[v] = std::min(lowlinks[v], indexes[w]);
            }
        }
    }

    // If v is a root node, pop the stack and generate an SCC
    if (lowlinks[v] == indexes[v]) {
        std::vector<std::string> comp;
        while (true) {
            std::string w = stack.back();
            stack.pop_back();
            onStack.erase(w);
            comp.push_back(std::move(w));
            if (w == v) break;
        }
        comps.push_back(std::move(comp));
    }
}

std::vector<std::vector<std::string>>
stronglyConnectedComponents(const Graph& g)
{
    const auto& nodes = g.getNodes();
    const auto& out   = g.getOutgoing();
    const auto& in    = g.getIncoming();
    const auto& edges = g.getEdges();

    std::unordered_set<std::string> visited;
    std::vector<std::string>        finishOrder;
    finishOrder.reserve(nodes.size());

    // 1) DFS1: on original graph to compute finish times
    std::function<void(const std::string&)> dfs1 = [&](auto const& u) {
        visited.insert(u);
        if (auto oit = out.find(u); oit != out.end()) {
            for (auto const& eid : oit->second) {
                if (auto eit = edges.find(eid); eit != edges.end()) {
                    auto const& v = eit->second.to;
                    if (!visited.count(v)) dfs1(v);
                }
            }
        }
        finishOrder.push_back(u);
    };

    for (auto const& [nid, node] : nodes) {
        if (!visited.count(nid)) dfs1(nid);
    }

    // 2) DFS2: on the *reversed* graph, in reverse finish order
    visited.clear();
    std::vector<std::vector<std::string>> components;
    components.reserve(nodes.size());

    std::function<void(const std::string&, std::vector<std::string>&)> dfs2 =
        [&](auto const& u, auto& comp) {
            visited.insert(u);
            comp.push_back(u);
            if (auto iit = in.find(u); iit != in.end()) {
                for (auto const& eid : iit->second) {
                    if (auto eit = edges.find(eid); eit != edges.end()) {
                        auto const& v = eit->second.from;
                        if (!visited.count(v)) dfs2(v, comp);
                    }
                }
            }
        };

    // Process nodes in decreasing finish time
    for (auto it = finishOrder.rbegin(); it != finishOrder.rend(); ++it) {
        if (!visited.count(*it)) {
            std::vector<std::string> comp;
            dfs2(*it, comp);
            components.push_back(std::move(comp));
        }
    }

    return components;
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

std::optional<std::vector<std::string>>
topologicalSort(const Graph& g)
{
    // 1) Gather all nodes and build in‐degree map
    const auto& nodes    = g.getNodes();
    const auto& outEdges = g.getOutgoing();
    const auto& edges    = g.getEdges();

    std::unordered_map<std::string,int> indegree;
    indegree.reserve(nodes.size());
    for (const auto& [nid, _] : nodes) {
        indegree[nid] = 0;
    }

    // For every outgoing edge u→v, increment indegree[v]
    for (const auto& [u, eids] : outEdges) {
        for (const auto& eid : eids) {
            auto it = edges.find(eid);
            if (it == edges.end()) continue;
            const auto& v = it->second.to;
            // only count if v is a known node
            if (indegree.count(v)) {
                indegree[v]++;
            }
        }
    }

    // 2) Initialize queue with all zero‐indegree nodes
    std::queue<std::string> q;
    for (const auto& [nid, deg] : indegree) {
        if (deg == 0) q.push(nid);
    }

    std::vector<std::string> order;
    order.reserve(nodes.size());

    // 3) Kahn’s algorithm
    while (!q.empty()) {
        auto u = q.front(); q.pop();
        order.push_back(u);

        // For each neighbor v of u
        auto oit = outEdges.find(u);
        if (oit == outEdges.end()) continue;
        for (const auto& eid : oit->second) {
            auto eit = edges.find(eid);
            if (eit == edges.end()) continue;
            const auto& v = eit->second.to;
            auto dit = indegree.find(v);
            if (dit == indegree.end()) continue;
            if (--(dit->second) == 0) {
                q.push(v);
            }
        }
    }

    // 4) If we ordered all nodes, succeed; else cycle detected
    if (order.size() == nodes.size()) {
        return order;
    } else {
        return std::nullopt;
    }
}

}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph

