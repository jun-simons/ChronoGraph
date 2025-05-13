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