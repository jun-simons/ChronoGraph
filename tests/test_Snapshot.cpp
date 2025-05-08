// tests/test_Snapshot.cpp

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <cstdint>

using namespace chronograph;

TEST(SnapshotBasic, NodesOnly) {
    Graph g;
    // Add two nodes at different times
    g.addNode("n1", {{"attr","A"}}, /*ts=*/100);
    g.addNode("n2", {{"attr","B"}}, /*ts=*/200);

    // Snapshot at t=150 should only see n1
    Snapshot s1(g, 150);
    const auto& nodes1 = s1.getNodes();
    EXPECT_EQ(nodes1.size(), 1u);
    EXPECT_TRUE(nodes1.count("n1"));
    EXPECT_EQ(nodes1.at("n1").attributes.at("attr"), "A");

    // Snapshot at t=200 should see both
    Snapshot s2(g, 200);
    const auto& nodes2 = s2.getNodes();
    EXPECT_EQ(nodes2.size(), 2u);
    EXPECT_TRUE(nodes2.count("n1"));
    EXPECT_TRUE(nodes2.count("n2"));
    EXPECT_EQ(nodes2.at("n2").attributes.at("attr"), "B");
}

TEST(SnapshotEdges, AddEdgeVisibility) {
    Graph g;
    // Prepare nodes
    g.addNode("n1", {}, /*ts=*/10);
    g.addNode("n2", {}, /*ts=*/10);

    // Add edge at ts=20
    g.addEdge("e1", "n1", "n2", {{"w","5"}}, /*ts=*/20);

    // Before edge, no edges
    Snapshot sBefore(g, 19);
    EXPECT_TRUE(sBefore.getEdges().empty());

    // At or after edge, should see it
    Snapshot sAfter(g, 20);
    const auto& edges = sAfter.getEdges();
    ASSERT_EQ(edges.size(), 1u);
    const Edge& e = edges.at("e1");
    
    EXPECT_EQ(e.from, "n1");
    EXPECT_EQ(e.to, "n2");
    EXPECT_EQ(e.attributes.at("w"), "5");
}

TEST(SnapshotDeletes, NodeDeletion) {
    Graph g;
    g.addNode("n1", {}, /*ts=*/1);
    g.addNode("n2", {}, /*ts=*/1);
    // delete n1 at ts=5
    g.delNode("n1", /*ts=*/5);

    // Snapshot just before deletion: n1 present
    Snapshot sPre(g, 4);
    EXPECT_TRUE(sPre.getNodes().count("n1"));
    EXPECT_TRUE(sPre.getNodes().count("n2"));

    // Snapshot at deletion time: n1 gone, n2 remains
    Snapshot sPost(g, 5);
    const auto& nodesPost = sPost.getNodes();
    EXPECT_EQ(nodesPost.count("n1"), 0u);
    EXPECT_EQ(nodesPost.count("n2"), 1u);
}

TEST(SnapshotUpdates, NodeAttributeUpdate) {
    Graph g;
    g.addNode("n1", {{"x","old"}}, /*ts=*/100);
    g.updateNode("n1", {{"x","new"}, {"y","added"}}, /*ts=*/200);

    // Before update
    Snapshot sOld(g, 150);
    ASSERT_TRUE(sOld.getNodes().count("n1"));
    EXPECT_EQ(sOld.getNodes().at("n1").attributes.at("x"), "old");
    EXPECT_EQ(sOld.getNodes().at("n1").attributes.count("y"), 0u);

    // After update
    Snapshot sNew(g, 200);
    ASSERT_TRUE(sNew.getNodes().count("n1"));
    EXPECT_EQ(sNew.getNodes().at("n1").attributes.at("x"), "new");
    EXPECT_EQ(sNew.getNodes().at("n1").attributes.at("y"), "added");
}

TEST(SnapshotEdgeDeletion, EdgeGoneAfterDelete) {
    Graph g;
    g.addNode("n1", {}, /*ts=*/10);
    g.addNode("n2", {}, /*ts=*/10);
    g.addEdge("e1", "n1", "n2", {{"w","5"}}, /*ts=*/20);

    // Delete the edge at ts=30
    g.delEdge("e1", /*ts=*/30);

    // Snapshot before deletion should still see the edge
    Snapshot sBefore(g, 29);
    ASSERT_EQ(sBefore.getEdges().size(), 1u);
    EXPECT_TRUE(sBefore.getEdges().count("e1"));

    // Snapshot at or after deletion should see no edges
    Snapshot sAfter(g, 30);
    EXPECT_TRUE(sAfter.getEdges().empty());
}

TEST(SnapshotEdgeUpdate, AttributesMergeCorrectly) {
    Graph g;
    g.addNode("n1", {}, /*ts=*/100);
    g.addNode("n2", {}, /*ts=*/100);
    g.addEdge("e1", "n1", "n2", {{"weight","5"}, {"type","orig"}}, /*ts=*/200);

    // Update the edge at ts=300
    g.updateEdge("e1", {{"weight","15"}, {"label","active"}}, /*ts=*/300);

    // Snapshot before update
    Snapshot sOld(g, 250);
    ASSERT_EQ(sOld.getEdges().size(), 1u);
    const auto& eOld = sOld.getEdges().at("e1");
    EXPECT_EQ(eOld.attributes.at("weight"), "5");     // original
    EXPECT_EQ(eOld.attributes.at("type"),   "orig");  // unchanged
    EXPECT_EQ(eOld.attributes.count("label"), 0u);    // not present yet

    // Snapshot at update time
    Snapshot sNew(g, 300);
    ASSERT_EQ(sNew.getEdges().size(), 1u);
    const auto& eNew = sNew.getEdges().at("e1");
    EXPECT_EQ(eNew.attributes.at("weight"), "15");    // updated
    EXPECT_EQ(eNew.attributes.at("type"),   "orig");  // still there
    EXPECT_EQ(eNew.attributes.at("label"),  "active");// new attr
}