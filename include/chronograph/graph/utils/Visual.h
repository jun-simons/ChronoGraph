#pragma once

#include <string>

namespace chronograph {
class Graph;

namespace graph {
namespace algorithms {
namespace utils {

/**
 * Render the given Graph as a Graphviz DOT string.
 * Nodes will be listed by their ID; edges by “from → to” with the edge ID as label.
 */
std::string toDot(const Graph& g);

}  // namespace utils
}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph
