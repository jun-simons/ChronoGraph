// examples/example_graph_demo.cpp

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <fstream>

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/algorithms/Connectivity.h>
#include <chronograph/graph/utils/Visual.h>

using namespace chronograph;
using namespace chronograph::graph::algorithms;
using namespace chronograph::graph::algorithms::utils;

int main() {
    Graph g;

    // 1) Build a more complex directed graph
    // Nodes: A,B,C,D,E,F,G,H
    int64_t ts = 1;
    for (auto id : {"A","B","C","D","E","F","G","H"}) {
        g.addNode(id, {{"label", std::string(id)}}, ts++);
    }

    // Edges: A->B, B->C, C->A (cycle);
    // D->E, E->F, F->D (another cycle);
    // C->D (bridge); G->H (singleton chain)
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","B","C",{}, ts++);
    g.addEdge("e3","C","A",{}, ts++);
    g.addEdge("e4","D","E",{}, ts++);
    g.addEdge("e5","E","F",{}, ts++);
    g.addEdge("e6","F","D",{}, ts++);
    g.addEdge("e7","C","D",{}, ts++);
    g.addEdge("e8","G","H",{}, ts++);

    // 2) Reachability tests
    std::cout << "Reachability:\n";
    std::cout << "  A→E ? " << (isReachable(g,"A","E") ? "yes":"no") << "\n";
    std::cout << "  E→A ? " << (isReachable(g,"E","A") ? "yes":"no") << "\n";
    std::cout << "  G→H ? " << (isReachable(g,"G","H") ? "yes":"no") << "\n";
    std::cout << "  H→G ? " << (isReachable(g,"H","G") ? "yes":"no") << "\n\n";

    // 3) Shortest paths
    std::cout << "Shortest paths:\n";
    auto p1 = shortestPath(g,"A","E");
    std::cout << "  A→E: ";
    if (p1.empty()) std::cout<<"<none>\n"; 
    else for (auto& n:p1) std::cout<<n<<" "; std::cout<<"\n";

    auto p2 = shortestPath(g,"G","H");
    std::cout << "  G→H: ";
    for (auto& n:p2) std::cout<<n<<" "; std::cout<<"\n\n";

    // 4) Connected components
    auto wcc = weaklyConnectedComponents(g);
    auto scc = stronglyConnectedComponents(g);

    std::cout<<"Weakly connected components:\n";
    for (auto& comp : wcc) {
        std::cout<<"  { ";
        for (auto& n:comp) std::cout<<n<<" ";
        std::cout<<"}\n";
    }
    std::cout<<"\nStrongly connected components:\n";
    for (auto& comp : scc) {
        std::cout<<"  { ";
        for (auto& n:comp) std::cout<<n<<" ";
        std::cout<<"}\n";
    }
    std::cout<<"\n";

    // 5) Topological sort
    // If graph contains cycle, will not run
    // (as is the case for this graph)
    auto topo = topologicalSort(g);
    std::cout<<"Topological sort: ";
    if (!topo) {
        std::cout<<"<cycle detected>\n\n";
    } else {
        for (auto& n:*topo) std::cout<<n<<" ";
        std::cout<<"\n\n";
    }

    // 6) Export to Graphviz DOT
    std::string dot = toDot(g);
    std::cout<<"DOT output written to graph.dot\n";
    std::ofstream out("graph.dot");
    out << dot;
    out.close();

    return 0;
}
