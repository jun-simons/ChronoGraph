// tests/test_Diff.cpp

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>
#include <gtest/gtest.h>
#include <map>
#include <string>

using namespace chronograph;

TEST(GraphDiff, AddUpdateDeleteNodesAndEdges) {
    Graph g;
    // t=1: add two nodes
    g.addNode("n1", {{"x","1"}}, 1);
    g.addNode("n2", {{"x","2"}}, 1);

    // t=2: add edge
    g.addEdge("e1", "n1", "n2", {{"w","5"}}, 2);

    // t=3: update node n1 and edge e1
    g.updateNode("n1", {{"x","10"}}, 3);
    g.updateEdge("e1", {{"w","50"}}, 3);

    // t=4: delete node n2 (this also deletes edge e1)
    g.delNode("n2", 4);

    // diff from t=1 -> t=3
    auto d1 = g.diff(1, 3);
    // nodes: n2 was added at 1? Actually both added at 1, so no additions.
    EXPECT_TRUE(d1.nodesAdded.empty());
    EXPECT_TRUE(d1.nodesRemoved.empty());
    ASSERT_EQ(d1.nodesUpdated.size(), 1u);
    EXPECT_EQ(d1.nodesUpdated[0].first.attributes.at("x"), "1");
    EXPECT_EQ(d1.nodesUpdated[0].second.attributes.at("x"), "10");

    // edges from 1 -> 3: e1 added and updated
    ASSERT_EQ(d1.edgesAdded.size(), 1u);
    EXPECT_EQ(d1.edgesAdded[0].id, "e1");
    // should not appear as updated from 1->3 
    // * (since it was both added and updated)
    EXPECT_TRUE(d1.edgesUpdated.empty()); 

    // diff from t=2 -> t=3
    auto d2 = g.diff(2,3);
    EXPECT_EQ(d2.edgesAdded.size(), 0u);
    // edges should now show as updated
    ASSERT_EQ(d2.edgesUpdated.size(), 1u);
    EXPECT_EQ(d2.edgesUpdated[0].first.attributes.at("w"), "5");
    EXPECT_EQ(d2.edgesUpdated[0].second.attributes.at("w"), "50");
    EXPECT_EQ(d2.edgesRemoved.size(), 0u);

    // diff from t=3 -> t=4
    auto d3 = g.diff(3, 4);
    ASSERT_EQ(d3.nodesRemoved.size(), 1u);
    EXPECT_EQ(d3.nodesRemoved[0], "n2");
    ASSERT_EQ(d3.edgesRemoved.size(), 1u);
    EXPECT_EQ(d3.edgesRemoved[0], "e1");
}
