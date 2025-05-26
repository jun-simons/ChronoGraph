// tests/test_Algorithms.cpp

#include <gtest/gtest.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>

using chronograph::Graph;
using chronograph::graph::algorithms::isReachable;
using chronograph::graph::algorithms::shortestPath;
using chronograph::graph::algorithms::isReachableAt;
using chronograph::graph::algorithms::isTimeRespectingReachable;

static int64_t ts = 1;

TEST(Reachability_OnGraph_LinearChain, MultiHop) {
    Graph g;
    // Build A → B → C
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);

    EXPECT_TRUE(isReachable(g, "A", "B"));
    EXPECT_TRUE(isReachable(g, "A", "C"));
    EXPECT_TRUE(isReachable(g, "B", "C"));
    EXPECT_FALSE(isReachable(g, "C", "A"));
    EXPECT_FALSE(isReachable(g, "B", "A"));
}

TEST(Reachability_OnGraph_Disconnected, NoPath) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    // no edge

    EXPECT_FALSE(isReachable(g, "A", "B"));
    EXPECT_FALSE(isReachable(g, "B", "A"));
}

TEST(Reachability_OnGraph_Cycle, HandlesCycle) {
    Graph g;
    // A → B → C → A
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);
    g.addEdge("e3", "C", "A", {}, ts++);

    EXPECT_TRUE(isReachable(g, "A", "C"));
    EXPECT_TRUE(isReachable(g, "B", "A"));
    EXPECT_TRUE(isReachable(g, "C", "B"));
}

TEST(Reachability_OnGraph_SelfReach, SelfNode) {
    Graph g;
    g.addNode("X", {}, ts++);
    // even without a self-edge, start==target should be true
    EXPECT_TRUE(isReachable(g, "X", "X"));
}

TEST(Reachability_OnGraph_SelfMissing, SelfMissing) {
    Graph g;
    // no nodes at all
    EXPECT_FALSE(isReachable(g, "Y", "Y"));
}

TEST(Reachability_OnGraph_Nonexistent, MissingNodes) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);

    // querying nodes that don't exist
    EXPECT_FALSE(isReachable(g, "A", "Z"));
    EXPECT_FALSE(isReachable(g, "Z", "A"));
    EXPECT_FALSE(isReachable(g, "M", "N"));
}

//
// Shortest-path tests
//
TEST(ShortestPath_OnGraph_LinearChain, PathExists) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);

    // A→B→C
    auto path = shortestPath(g, "A", "C");
    EXPECT_EQ(path, (std::vector<std::string>{"A","B","C"}));

    // direct neighbor
    path = shortestPath(g, "A", "B");
    EXPECT_EQ(path, (std::vector<std::string>{"A","B"}));

    // start equals target
    path = shortestPath(g, "B", "B");
    EXPECT_EQ(path, (std::vector<std::string>{"B"}));

    // no path backwards
    path = shortestPath(g, "C", "A");
    EXPECT_TRUE(path.empty());
}

TEST(ShortestPath_OnGraph_Disconnected, NoPath) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    auto path = shortestPath(g, "A", "B");
    EXPECT_TRUE(path.empty());
}

TEST(ShortestPath_OnGraph_Cycle, HandlesCycle) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "C", {}, ts++);
    g.addEdge("e3", "C", "A", {}, ts++);

    // shortest from A to C is A->B->C
    auto path = shortestPath(g, "A", "C");
    EXPECT_EQ(path, (std::vector<std::string>{"A","B","C"}));

    // C->A direct via C->A edge
    path = shortestPath(g, "C", "A");
    EXPECT_EQ(path, (std::vector<std::string>{"C","A"}));
}

TEST(ShortestPath_OnGraph_MultiplePaths, ChoosesShortest) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addNode("D", {}, ts++);
    // Two paths A->B->D and A->C->D
    g.addEdge("e1", "A", "B", {}, ts++);
    g.addEdge("e2", "B", "D", {}, ts++);
    g.addEdge("e3", "A", "C", {}, ts++);
    g.addEdge("e4", "C", "D", {}, ts++);

    auto path = shortestPath(g, "A", "D");
    // Both have length 2; BFS explores B before C, so path should be A,B,D
    EXPECT_EQ(path, (std::vector<std::string>{"A","B","D"}));
}

TEST(ShortestPath_OnGraph_Nonexistent, MissingNodes) {
    Graph g;
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    auto path = shortestPath(g, "A", "Z");
    EXPECT_TRUE(path.empty());
    path = shortestPath(g, "X", "A");
    EXPECT_TRUE(path.empty());
    path = shortestPath(g, "X", "Y");
    EXPECT_TRUE(path.empty());
}

TEST(ReachabilityAtT_OnGraph, TimelineScenarios) {
    // Test isReachableAt with 4 even timestamps
    Graph g;
    // t=1: create three nodes A,B,C
    g.addNode("A", {}, 1);
    g.addNode("B", {}, 1);
    g.addNode("C", {}, 1);
    // t=2: add edge A→B
    g.addEdge("e1", "A", "B", {}, 2);
    // t=3: add edge B→C
    g.addEdge("e2", "B", "C", {}, 3);
    // t=4: delete edge e2
    g.delEdge("e2", 4);

    // before any edges: no reachability
    EXPECT_FALSE(isReachableAt(g, "A", "B", 1));
    EXPECT_FALSE(isReachableAt(g, "A", "C", 1));

    // after A->B only
    EXPECT_TRUE (isReachableAt(g, "A", "B", 2));
    EXPECT_FALSE(isReachableAt(g, "A", "C", 2));

    // after B->C: A->C via A->B->C
    EXPECT_TRUE (isReachableAt(g, "A", "C", 3));
    EXPECT_TRUE (isReachableAt(g, "B", "C", 3));

    // immediately after deleting B->C: A->C no longer reachable
    EXPECT_FALSE(isReachableAt(g, "A", "C", 4));
    // but A->B still works
    EXPECT_TRUE (isReachableAt(g, "A", "B", 4));
}

TEST(TimeRespectingReachability_IncreasingTimestamps, MultiHopSuccess) {
    Graph g;
    // nodes
    g.addNode("A", {}, 1);
    g.addNode("B", {}, 1);
    g.addNode("C", {}, 1);
    // edges with non-decreasing times
    g.addEdge("e1", "A", "B", {}, 5);   // A→B at t=5
    g.addEdge("e2", "B", "C", {}, 10);  // B→C at t=10

    // should be reachable via A→B→C
    EXPECT_TRUE(isTimeRespectingReachable(g, "A", "C"));
    // direct neighbor always OK
    EXPECT_TRUE(isTimeRespectingReachable(g, "A", "B"));
}

TEST(TimeRespectingReachability_DecreasingTimestamps, MultiHopFailure) {
    Graph g;
    g.addNode("A", {}, 1);
    g.addNode("B", {}, 1);
    g.addNode("C", {}, 1);
    // B->C created first, then A→B
    g.addEdge("e1", "B", "C", {}, 5);   // B->C at t=5
    g.addEdge("e2", "A", "B", {}, 10);  // A->B at t=10

    // from A reach B at t=10, but B→C is t=5<10, so cannot continue
    EXPECT_FALSE(isTimeRespectingReachable(g, "A", "C"));
    // A->B itself is OK
    EXPECT_TRUE(isTimeRespectingReachable(g, "A", "B"));
}

TEST(TimeRespectingReachability_SameTimestamp, EqualTimes) {
    Graph g;
    g.addNode("X", {}, 1);
    g.addNode("Y", {}, 1);
    // both edges at same timestamp
    g.addEdge("e1", "X", "Y", {}, 7);
    g.addEdge("e2", "Y", "Z", {}, 7);
    g.addNode("Z", {}, 1);

    // Y->Z added before querying, but createdTimestamp == lastTs => allowed
    EXPECT_TRUE(isTimeRespectingReachable(g, "X", "Z"));
}

TEST(TimeRespectingReachability_SelfAndMissing, EdgeCases) {
    Graph g;
    g.addNode("N", {}, 1);

    // self is always reachable if node exists
    EXPECT_TRUE(isTimeRespectingReachable(g, "N", "N"));
    // missing nodes
    EXPECT_FALSE(isTimeRespectingReachable(g, "N", "M"));
    EXPECT_FALSE(isTimeRespectingReachable(g, "M", "N"));
    EXPECT_FALSE(isTimeRespectingReachable(g, "U", "V"));
}

