// include/chronograph/graph/algorithms/Paths.h
#pragma once

#include <string>
#include <vector>

namespace chronograph {

class Graph;

namespace graph {
namespace algorithms {

/**
 * Returns true if `target` is reachable from `start` in the given graph.
 */
bool isReachable(const Graph& g,
                 const std::string& start,
                 const std::string& target);

/**
 * Compute an unweighted shortest path from `start` to `target` in `g`.
 * Returns the sequence of node IDs [start, ..., target].
 * Empty vector if no path exists (or if either node is missing).
 */
std::vector<std::string> shortestPath(const Graph& g,
    const std::string& start,
    const std::string& target);

}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
