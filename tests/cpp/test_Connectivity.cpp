// tests/test_Connectivity.cpp

#include <gtest/gtest.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Connectivity.h>
#include <algorithm>  // for std::sort
#include <vector>
#include <string>

using chronograph::Graph;
using chronograph::graph::algorithms::weaklyConnectedComponents;
using chronograph::graph::algorithms::stronglyConnectedComponents;
using chronograph::graph::algorithms::hasCycle;



static int64_t ts = 1;

// Helper to sort components (each comp and the list of comps)
static std::vector<std::vector<std::string>>
sortComps(std::vector<std::vector<std::string>> comps) {
    for (auto& comp : comps) {
        std::sort(comp.begin(), comp.end());
    }
    std::sort(comps.begin(), comps.end());
    return comps;
}

// Helper to sort components for comparison
static std::vector<std::vector<std::string>>
sortComponents(std::vector<std::vector<std::string>> comps) {
    for (auto& comp : comps) {
        std::sort(comp.begin(), comp.end());
    }
    std::sort(comps.begin(), comps.end());
    return comps;
}

TEST(Connectivity_EmptyGraph, NoNodes) {
    Graph g;
    auto comps = weaklyConnectedComponents(g);
    EXPECT_TRUE(comps.empty());
}

TEST(Connectivity_SingleNode, OneComponent) {
    Graph g;
    g.addNode("A", {}, ts++);
    auto comps = weaklyConnectedComponents(g);
    // Should be exactly one component ["A"]
    auto sorted = sortComponents(comps);
    std::vector<std::vector<std::string>> expected{{"A"}};
    EXPECT_EQ(sorted, expected);
}

TEST(Connectivity_DisconnectedNodes, MultipleSingletons) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    auto comps = weaklyConnectedComponents(g);
    // Two singleton components: ["A"], ["B"]
    auto sorted = sortComponents(comps);
    std::vector<std::vector<std::string>> expected{{"A"}, {"B"}};
    EXPECT_EQ(sorted, expected);
}

TEST(Connectivity_PairConnected, OneComponent) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    auto comps = weaklyConnectedComponents(g);
    // A and B should be in the same component
    auto sorted = sortComponents(comps);
    std::vector<std::vector<std::string>> expected{{"A", "B"}};
    EXPECT_EQ(sorted, expected);
}

TEST(Connectivity_ComplexGraph, TwoComponents) {
    Graph g;
    // Component 1: cycle A→B→C→A
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);
    g.addEdge("e3", "C", "A", {}, ts++);
    // Component 2: path D→E
    g.addNode("D", {}, ts++);
    g.addNode("E", {}, ts++);
    g.addEdge("e4", "D", "E", {}, ts++);

    auto comps = weaklyConnectedComponents(g);
    // Expect two components: {A,B,C} and {D,E}
    auto sorted = sortComponents(comps);
    std::vector<std::vector<std::string>> expected{
        {"A","B","C"},
        {"D","E"}
    };
    EXPECT_EQ(sorted, expected);
}

// -- Strongly Connected Components tests --

TEST(Connectivity_Strong_EmptyGraph, NoNodes) {
    Graph g;
    auto comps = stronglyConnectedComponents(g);
    EXPECT_TRUE(comps.empty());
}

TEST(Connectivity_Strong_SingleNode, OneComponent) {
    Graph g;
    g.addNode("X", {}, ts++);
    auto comps = stronglyConnectedComponents(g);
    auto sorted = sortComps(comps);
    EXPECT_EQ(sorted, (std::vector<std::vector<std::string>>{{"X"}}));
}

TEST(Connectivity_Strong_DirectedChain, AllSingletons) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","B","C",{}, ts++);

    auto comps = stronglyConnectedComponents(g);
    auto sorted = sortComps(comps);
    EXPECT_EQ(sorted, (std::vector<std::vector<std::string>>{{"A"},{"B"},{"C"}}));
}

TEST(Connectivity_Strong_SimpleCycle, OneComponent) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","B","C",{}, ts++);
    g.addEdge("e3","C","A",{}, ts++);

    auto comps = stronglyConnectedComponents(g);
    auto sorted = sortComps(comps);
    EXPECT_EQ(sorted, (std::vector<std::vector<std::string>>{{"A","B","C"}}));
}

TEST(Connectivity_Strong_MixedGraph, MultipleSCCs) {
    Graph g;
    // cycle 1: A<->B
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","B","A",{}, ts++);
    // cycle 2: C->D->E->C
    g.addNode("C", {}, ts++);
    g.addNode("D", {}, ts++);
    g.addNode("E", {}, ts++);
    g.addEdge("e3","C","D",{}, ts++);
    g.addEdge("e4","D","E",{}, ts++);
    g.addEdge("e5","E","C",{}, ts++);
    // a bridge from B->C (connects the two cycles but doesn't merge them)
    g.addEdge("e6","B","C",{}, ts++);

    auto comps = stronglyConnectedComponents(g);
    auto sorted = sortComps(comps);
    EXPECT_EQ(sorted, (std::vector<std::vector<std::string>>{
        {"A","B"},
        {"C","D","E"}
    }));
}


static int64_t ts_cycle = 1;

TEST(CycleDetection_EmptyGraph, NoNodes) {
    Graph g;
    EXPECT_FALSE(hasCycle(g));
}

TEST(CycleDetection_SingleNodeNoEdges, NoCycle) {
    Graph g;
    g.addNode("A", {}, ts_cycle++);
    EXPECT_FALSE(hasCycle(g));
}

TEST(CycleDetection_SelfLoop, SingleNodeCycle) {
    Graph g;
    g.addNode("A", {}, ts_cycle++);
    g.addEdge("e1","A","A",{}, ts_cycle++);
    EXPECT_TRUE(hasCycle(g));
}

TEST(CycleDetection_LinearChain, NoCycle) {
    Graph g;
    g.addNode("A", {}, ts_cycle++);
    g.addNode("B", {}, ts_cycle++);
    g.addNode("C", {}, ts_cycle++);
    g.addEdge("e1","A","B",{}, ts_cycle++);
    g.addEdge("e2","B","C",{}, ts_cycle++);
    EXPECT_FALSE(hasCycle(g));
}

TEST(CycleDetection_SimpleTwoNodeCycle, Cycle) {
    Graph g;
    g.addNode("X", {}, ts_cycle++);
    g.addNode("Y", {}, ts_cycle++);
    g.addEdge("e1","X","Y",{}, ts_cycle++);
    g.addEdge("e2","Y","X",{}, ts_cycle++);
    EXPECT_TRUE(hasCycle(g));
}

TEST(CycleDetection_ThreeNodeCycle, Cycle) {
    Graph g;
    g.addNode("A", {}, ts_cycle++);
    g.addNode("B", {}, ts_cycle++);
    g.addNode("C", {}, ts_cycle++);
    // A → B → C → A
    g.addEdge("e1","A","B",{}, ts_cycle++);
    g.addEdge("e2","B","C",{}, ts_cycle++);
    g.addEdge("e3","C","A",{}, ts_cycle++);
    EXPECT_TRUE(hasCycle(g));
}

TEST(CycleDetection_MixedComponents, CycleExists) {
    Graph g;
    // Component 1: acyclic
    g.addNode("P", {}, ts_cycle++);
    g.addNode("Q", {}, ts_cycle++);
    g.addEdge("e1","P","Q",{}, ts_cycle++);
    // Component 2: cyclic
    g.addNode("R", {}, ts_cycle++);
    g.addNode("S", {}, ts_cycle++);
    g.addEdge("e2","R","S",{}, ts_cycle++);
    g.addEdge("e3","S","R",{}, ts_cycle++);
    EXPECT_TRUE(hasCycle(g));
}

using chronograph::graph::algorithms::topologicalSort;

// Helper: verify that 'order' is a valid topological ordering of g
bool isValidTopoOrder(const std::vector<std::string>& order, const Graph& g) {
    // map node -> its position in 'order'
    std::unordered_map<std::string,size_t> pos;
    for (size_t i = 0; i < order.size(); ++i) {
        pos[order[i]] = i;
    }
    // every node must appear exactly once
    const auto& nodes = g.getNodes();
    if (pos.size() != nodes.size()) return false;
    for (auto& [nid, _] : nodes) {
        if (!pos.count(nid)) return false;
    }
    // for every edge u->v, pos[u] < pos[v]
    const auto& out = g.getOutgoing();
    const auto& edges = g.getEdges();
    for (auto& [u, eids] : out) {
        for (auto& eid : eids) {
            auto it = edges.find(eid);
            if (it == edges.end()) continue;
            auto& v = it->second.to;
            if (pos[u] >= pos[v]) return false;
        }
    }
    return true;
}

TEST(TopologicalSort_EmptyGraph, ReturnsEmpty) {
    Graph g;
    auto opt = topologicalSort(g);
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->empty());
}

TEST(TopologicalSort_SingleNode, ReturnsThatNode) {
    Graph g;
    g.addNode("A", {}, ts++);
    auto opt = topologicalSort(g);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, std::vector<std::string>{"A"});
}

TEST(TopologicalSort_SimpleChain, AtoC) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);
    auto opt = topologicalSort(g);
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(isValidTopoOrder(*opt, g));
    // Optionally check exactly one possible order
    EXPECT_EQ(*opt, (std::vector<std::string>{"A","B","C"}));
}

TEST(TopologicalSort_BranchedDAG, ValidOrder) {
    Graph g;
    // A->B, A->C, B->D, C->D
    for (auto id : {"A","B","C","D"}) g.addNode(id, {}, ts++);
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","A","C",{}, ts++);
    g.addEdge("e3","B","D",{}, ts++);
    g.addEdge("e4","C","D",{}, ts++);
    auto opt = topologicalSort(g);
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(isValidTopoOrder(*opt, g));
    // must start with A and end with D
    EXPECT_EQ((*opt).front(), "A");
    EXPECT_EQ((*opt).back(),  "D");
}

TEST(TopologicalSort_WithCycle, DetectsCycle) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    // cycle A->B->C->A
    g.addEdge("e1","A","B",{}, ts++);
    g.addEdge("e2","B","C",{}, ts++);
    g.addEdge("e3","C","A",{}, ts++);
    auto opt = topologicalSort(g);
    EXPECT_FALSE(opt.has_value());
}