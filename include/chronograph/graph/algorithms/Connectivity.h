#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <optional>

namespace chronograph {
class Graph;

namespace graph {
namespace algorithms {

/// Compute the weakly-connected components of a directed graph.
/// Treats edges as undirected. Returns a vector of components,
/// each component is a list of node-IDs.
std::vector<std::vector<std::string>>
weaklyConnectedComponents(const Graph& g);

/**
 * Compute the strongly‐connected components of a directed graph.
 * Returns a vector of components, each a list of node‐IDs.
 */
std::vector<std::vector<std::string>>
stronglyConnectedComponents(const Graph& g);

/**
 * Return true if the directed graph contains any cycle.
 */
bool hasCycle(const Graph& g);

/**
 * Perform a topological sort of the directed graph.
 * If the graph is acyclic, returns a vector of node‐IDs in topological order.
 * If there is a cycle, returns std::nullopt.
 */
std::optional<std::vector<std::string>>
topologicalSort(const Graph& g);


}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph