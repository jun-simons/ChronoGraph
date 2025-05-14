#include <chronograph/graph/utils/Visual.h>
#include <chronograph/graph/Graph.h>
#include <sstream>

namespace chronograph {
namespace graph {
namespace algorithms {
namespace utils {

std::string toDot(const Graph& g) {
    const auto& nodes     = g.getNodes();
    const auto& outEdges  = g.getOutgoing();
    const auto& edges     = g.getEdges();

    std::ostringstream ss;
    ss << "digraph ChronoGraph {\n";
    // 1) Emit node declarations
    for (const auto& [nid, node] : nodes) {
        ss << "  \"" << nid << "\";\n";
    }
    ss << "\n";
    // 2) Emit edges
    for (const auto& [u, eids] : outEdges) {
        for (const auto& eid : eids) {
            auto it = edges.find(eid);
            if (it == edges.end()) continue;
            const auto& e = it->second;
            ss << "  \"" << e.from << "\" -> \"" << e.to
               << "\" [label=\"" << eid << "\"];\n";
        }
    }
    ss << "}\n";
    return ss.str();
}

}  // namespace utils
}  // namespace algorithms
}  // namespace graph
}  // namespace chronograph