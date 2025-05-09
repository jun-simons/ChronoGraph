// include/chronograph/graph/algorithms/Paths.h
#pragma once

#include <string>

namespace chronograph {

class Graph;

namespace graph {
namespace algorithms {

/// Returns true if `target` is reachable from `start` in the given graph,
/// considering only the current live edges.
bool isReachable(const Graph& g,
                 const std::string& start,
                 const std::string& target);

}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
