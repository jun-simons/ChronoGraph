// include/chronograph/graph/algorithms/Paths.h
#pragma once

#include <string>
#include <vector>
#include <cstdint>

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

/**
* Returns true if `target` is reachable from `start` in `g` *as of* `timestamp`.
* Internally takes a Snapshot at time T and runs reachability on that snapshot.
*/
bool isReachableAt(const Graph& g,
    const std::string& start,
    const std::string& target,
    std::int64_t timestamp);

/**
 * Returns true if `target` is reachable from `start` in the given graph,
 * considering only paths whose edge‐creation timestamps never decrease.
 */
bool isTimeRespectingReachable(const Graph& g,
    const std::string& start,
    const std::string& target);


/**
 * Compute a weighted shortest path from `start` to `target` in `g`.
 * - `weightKey` is the attribute name in each Edge::attributes map that stores
 *    a numeric weight (parsed as `double`).
 * - If an edge does not contain `weightKey` or if the attribute is not a valid
 *   number, that edge is skipped.
 * Returns the sequence of node IDs [start, ..., target].
 * Returns an empty vector if no path exists (or if start/target missing).
 */
std::vector<std::string> dijkstra(
    const Graph& g,
    const std::string& start,
    const std::string& target,
    const std::string& weightKey);



}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
