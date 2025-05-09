// // tests/test_Algorithms.cpp

// #include <gtest/gtest.h>
// #include <chronograph/graph/Graph.h>
// #include <chronograph/graph/algorithms/Paths.h>

// using chronograph::Graph;
// using chronograph::graph::algorithms::isReachable;

// static int64_t ts = 1;

// TEST(Reachability_OnGraph_LinearChain, MultiHop) {
//     Graph g;
//     // Build A → B → C
//     g.addNode("A", {}, ts++);
//     g.addNode("B", {}, ts++);
//     g.addNode("C", {}, ts++);
//     g.addEdge("e1", "A", "B", {}, ts++);
//     g.addEdge("e2", "B", "C", {}, ts++);

//     EXPECT_TRUE(isReachable(g, "A", "B"));
//     EXPECT_TRUE(isReachable(g, "A", "C"));
//     EXPECT_TRUE(isReachable(g, "B", "C"));
//     EXPECT_FALSE(isReachable(g, "C", "A"));
//     EXPECT_FALSE(isReachable(g, "B", "A"));
// }

// TEST(Reachability_OnGraph_Disconnected, NoPath) {
//     Graph g;
//     g.addNode("A", {}, ts++);
//     g.addNode("B", {}, ts++);
//     // no edge

//     EXPECT_FALSE(isReachable(g, "A", "B"));
//     EXPECT_FALSE(isReachable(g, "B", "A"));
// }

// TEST(Reachability_OnGraph_Cycle, HandlesCycle) {
//     Graph g;
//     // A → B → C → A
//     g.addNode("A", {}, ts++);
//     g.addNode("B", {}, ts++);
//     g.addNode("C", {}, ts++);
//     g.addEdge("e1", "A", "B", {}, ts++);
//     g.addEdge("e2", "B", "C", {}, ts++);
//     g.addEdge("e3", "C", "A", {}, ts++);

//     EXPECT_TRUE(isReachable(g, "A", "C"));
//     EXPECT_TRUE(isReachable(g, "B", "A"));
//     EXPECT_TRUE(isReachable(g, "C", "B"));
// }

// TEST(Reachability_OnGraph_SelfReach, SelfNode) {
//     Graph g;
//     g.addNode("X", {}, ts++);
//     // even without a self-edge, start==target should be true
//     EXPECT_TRUE(isReachable(g, "X", "X"));
// }

// TEST(Reachability_OnGraph_SelfMissing, SelfMissing) {
//     Graph g;
//     // no nodes at all
//     EXPECT_FALSE(isReachable(g, "Y", "Y"));
// }

// TEST(Reachability_OnGraph_Nonexistent, MissingNodes) {
//     Graph g;
//     g.addNode("A", {}, ts++);
//     g.addNode("B", {}, ts++);
//     g.addEdge("e1", "A", "B", {}, ts++);

//     // querying nodes that don't exist
//     EXPECT_FALSE(isReachable(g, "A", "Z"));
//     EXPECT_FALSE(isReachable(g, "Z", "A"));
//     EXPECT_FALSE(isReachable(g, "M", "N"));
// }
