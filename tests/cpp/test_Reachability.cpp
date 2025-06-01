// tests/test_Algorithms.cpp

#include <gtest/gtest.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>

using chronograph::Graph;
using chronograph::graph::algorithms::isReachable;
using chronograph::graph::algorithms::shortestPath;
using chronograph::graph::algorithms::isReachableAt;
using chronograph::graph::algorithms::isTimeRespectingReachable;
using chronograph::graph::algorithms::dijkstra;

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


// ─────────────────────────────────────────────────────────────────────────────
// Dijkstra‐based weighted shortest‐path tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(Dijkstra_OnGraph_LinearChain, BasicWeights) {
    Graph g;
    int64_t ts = 1;

    // Create nodes A, B, C
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);

    // A -> B with weight=2.0, B -> C with weight=3.0
    g.addEdge("e1", "A", "B", {{"cost","2.0"}}, ts++);
    g.addEdge("e2", "B", "C", {{"cost","3.0"}}, ts++);

    // Expect path A->B->C
    auto path = dijkstra(g, "A", "C", "cost");
    std::vector<std::string> expected1 = {"A","B","C"};
    EXPECT_EQ(path, expected1);

    // Direct neighbor A->B
    path = dijkstra(g, "A", "B", "cost");
    std::vector<std::string> expected2 = {"A","B"};
    EXPECT_EQ(path, expected2);

    // Self‐path (A->A)
    path = dijkstra(g, "A", "A", "cost");
    std::vector<std::string> expected3 = {"A"};
    EXPECT_EQ(path, expected3);

    // No path backwards (C->A)
    path = dijkstra(g, "C", "A", "cost");
    EXPECT_TRUE(path.empty());
}

TEST(Dijkstra_OnGraph_MultiplePaths, ChoosesLowestTotalCost) {
    Graph g;
    int64_t ts = 10;

    // Nodes: A, B, C, D
    g.addNode("A", {}, ts++);
    g.addNode("B", {}, ts++);
    g.addNode("C", {}, ts++);
    g.addNode("D", {}, ts++);

    // Two routes:
    //   A->B (1.0), B->D (1.0)  total=2.0
    //   A->C (2.0), C->D (1.0)  total=3.0
    g.addEdge("e1", "A", "B", {{"wt","1.0"}}, ts++);
    g.addEdge("e2", "B", "D", {{"wt","1.0"}}, ts++);
    g.addEdge("e3", "A", "C", {{"wt","2.0"}}, ts++);
    g.addEdge("e4", "C", "D", {{"wt","1.0"}}, ts++);

    // Expect the lower‐cost path A->B->D
    auto path = dijkstra(g, "A", "D", "wt");
    std::vector<std::string> expected = {"A","B","D"};
    EXPECT_EQ(path, expected);
}



TEST(Dijkstra_OnGraph_MissingOrInvalidWeight, SkipsEdgesWithoutValidKey) {
    Graph g;
    int64_t ts = 100;

    // Nodes: X, Y, Z
    g.addNode("X", {}, ts++);
    g.addNode("Y", {}, ts++);
    g.addNode("Z", {}, ts++);

    // X→Y (no “cost” attribute), Y→Z (cost=5.0)
    // Also X→Z direct but with invalid (“abc”) weight
    g.addEdge("e1", "X", "Y", {{"other","1.0"}}, ts++);
    g.addEdge("e2", "Y", "Z", {{"cost","5.0"}}, ts++);
    g.addEdge("e3", "X", "Z", {{"cost","abc"}}, ts++);

    // The only valid route is X→Y→Z because X→Y lacks “cost” and X→Z is unparsable.
    auto path = dijkstra(g, "X", "Z", "cost");
    std::vector<std::string> expected = {"X","Y","Z"};
    EXPECT_EQ(path, expected);

    // If we ask for a nonexistent key (“weight”), everything is invalid → empty
    auto path2 = dijkstra(g, "X", "Z", "weight");
    EXPECT_TRUE(path2.empty());
}

TEST(Dijkstra_OnGraph_NonexistentOrDisconnected, VariousEdgeCases) {
    Graph g;
    int64_t ts = 50;

    // Single node “Solo”
    g.addNode("Solo", {}, ts++);

    // Path from Solo→Solo should return {"Solo"} even though no edges exist
    auto path_self = dijkstra(g, "Solo", "Solo", "w");
    std::vector<std::string> solo_expected = {"Solo"};
    EXPECT_EQ(path_self, solo_expected);

    // Nonexistent start node
    auto path_missingStart = dijkstra(g, "Foo", "Solo", "w");
    EXPECT_TRUE(path_missingStart.empty());

    // Nonexistent target node
    auto path_missingTarget = dijkstra(g, "Solo", "Bar", "w");
    EXPECT_TRUE(path_missingTarget.empty());

    // Add another node “Isle” but do not connect
    g.addNode("Isle", {}, ts++);
    auto path_disconnected = dijkstra(g, "Solo", "Isle", "w");
    EXPECT_TRUE(path_disconnected.empty());
}

TEST(Dijkstra_OnGraph_MultiEdgeSameEndpoints, PicksMinWeight) {
    Graph g;
    int64_t ts = 200;

    // Nodes: P, Q, R
    g.addNode("P", {}, ts++);
    g.addNode("Q", {}, ts++);
    g.addNode("R", {}, ts++);

    // Two edges from P→Q: one with cost=10, another with cost=1
    g.addEdge("eA", "P", "Q", {{"w","10"}}, ts++);
    g.addEdge("eB", "P", "Q", {{"w","1"}}, ts++);

    // Then Q→R cost=2
    g.addEdge("eC", "Q", "R", {{"w","2"}}, ts++);

    // If dijkstra picks the minimal P→Q=1.0, total cost P→Q→R = 1+2 = 3
    auto path = dijkstra(g, "P", "R", "w");
    std::vector<std::string> expected = {"P","Q","R"};
    EXPECT_EQ(path, expected);
}