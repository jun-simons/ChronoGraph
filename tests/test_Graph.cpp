// tests/test_Graph.cpp

#include "chronograph/Graph.h"
#include <gtest/gtest.h>

using namespace chronograph;

TEST(GraphAddNode, BasicSmoke) {
    Graph g;
    std::map<std::string, std::string> attrs = {
        {"name", "Alice"},
        {"role", "admin"}
    };
    int64_t ts = 1234567;  // arbitrary timestamp

    // Before adding, graph should be empty
    EXPECT_TRUE(g.getEventLog().empty());
    EXPECT_TRUE(g.getNodes().empty());
    EXPECT_TRUE(g.getOutgoing().empty());
    EXPECT_TRUE(g.getIncoming().empty());

    // Add a node
    g.addNode("node1", attrs, ts);

    // Event‐log should have exactly one entry
    ASSERT_EQ(g.getEventLog().size(), 1u);
    const Event& e = g.getEventLog().front();
    EXPECT_EQ(e.type, EventType::ADD_NODE);
    EXPECT_EQ(e.entityId, "node1");
    EXPECT_EQ(e.timestamp, ts);
    EXPECT_EQ(e.payload.at("name"), "Alice");
    EXPECT_EQ(e.payload.at("role"), "admin");

    // Node store should now contain "node1"
    ASSERT_EQ(g.getNodes().count("node1"), 1u);
    const Node& n = g.getNodes().at("node1");
    EXPECT_EQ(n.id, "node1");
    EXPECT_EQ(n.attributes.at("name"), "Alice");
    EXPECT_EQ(n.attributes.at("role"), "admin");

    // Adjacency lists for node1 should exist and be empty
    ASSERT_EQ(g.getOutgoing().count("node1"), 1u);
    EXPECT_TRUE(g.getOutgoing().at("node1").empty());
    ASSERT_EQ(g.getIncoming().count("node1"), 1u);
    EXPECT_TRUE(g.getIncoming().at("node1").empty());
}
