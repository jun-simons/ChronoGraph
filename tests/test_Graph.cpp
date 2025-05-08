// tests/test_Graph.cpp

#include <chronograph/graph/Graph.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <cstdint>

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

TEST(GraphAddEdge, BasicSmoke) {
    Graph g;
    // First add two nodes
    g.addNode("n1", {{"label","N1"}}, 100);
    g.addNode("n2", {{"label","N2"}}, 200);

    // Add an edge between them
    std::map<std::string,std::string> edgeAttrs = {{"weight","5"}};
    int64_t tsEdge = 300;
    g.addEdge("e1", "n1", "n2", edgeAttrs, tsEdge);

    // We should now have 3 events: ADD_NODE x2, ADD_EDGE
    ASSERT_EQ(g.getEventLog().size(), 3u);
    const Event& e = g.getEventLog().back();
    EXPECT_EQ(e.type, EventType::ADD_EDGE);
    EXPECT_EQ(e.entityId, "e1");
    EXPECT_EQ(e.timestamp, tsEdge);
    EXPECT_EQ(e.payload.at("weight"), "5");

    // Edge store should contain "e1"
    ASSERT_EQ(g.getEdges().count("e1"), 1u);
    const Edge& edge = g.getEdges().at("e1");
    EXPECT_EQ(edge.id, "e1");
    EXPECT_EQ(edge.from, "n1");
    EXPECT_EQ(edge.to, "n2");
    EXPECT_EQ(edge.attributes.at("weight"), "5");

    // Adjacency should reflect the new edge
    ASSERT_EQ(g.getOutgoing().at("n1").size(), 1u);
    EXPECT_EQ(g.getOutgoing().at("n1")[0], "e1");
    ASSERT_EQ(g.getIncoming().at("n2").size(), 1u);
    EXPECT_EQ(g.getIncoming().at("n2")[0], "e1");
}

TEST(GraphDelEdge, BasicSmoke) {
    Graph g;
    // Setup
    g.addNode("n1", {}, 10);
    g.addNode("n2", {}, 20);
    g.addEdge("e1", "n1", "n2", {}, 30);

    // Delete the edge
    int64_t tsDel = 40;
    g.delEdge("e1", tsDel);

    // Events: ADD_NODE, ADD_NODE, ADD_EDGE, DEL_EDGE
    ASSERT_EQ(g.getEventLog().size(), 4u);
    const Event& e = g.getEventLog().back();
    EXPECT_EQ(e.type, EventType::DEL_EDGE);
    EXPECT_EQ(e.entityId, "e1");
    EXPECT_EQ(e.timestamp, tsDel);

    // Edge store should no longer contain "e1"
    EXPECT_EQ(g.getEdges().count("e1"), 0u);

    // Adjacency lists should no longer reference "e1"
    EXPECT_TRUE(g.getOutgoing().at("n1").empty());
    EXPECT_TRUE(g.getIncoming().at("n2").empty());
}

TEST(GraphDelNode, BasicSmoke) {
    Graph g;
    // Setup two nodes and one connecting edge
    g.addNode("n1", {}, 1);
    g.addNode("n2", {}, 2);
    g.addEdge("e1", "n1", "n2", {}, 3);

    // Delete node n1 (this should also delete edge e1)
    int64_t tsDelNode = 4;
    g.delNode("n1", tsDelNode);

    // Events: ADD_NODE, ADD_NODE, ADD_EDGE, DEL_NODE, DEL_EDGE
    ASSERT_EQ(g.getEventLog().size(), 5u);

    // 4th event is DEL_NODE for n1
    const Event& eNode = g.getEventLog()[3];
    EXPECT_EQ(eNode.type, EventType::DEL_NODE);
    EXPECT_EQ(eNode.entityId, "n1");
    EXPECT_EQ(eNode.timestamp, tsDelNode);

    // 5th event is DEL_EDGE for e1
    const Event& eEdge = g.getEventLog()[4];
    EXPECT_EQ(eEdge.type, EventType::DEL_EDGE);
    EXPECT_EQ(eEdge.entityId, "e1");
    EXPECT_EQ(eEdge.timestamp, tsDelNode);

    // Node store: n1 gone, n2 remains
    EXPECT_EQ(g.getNodes().count("n1"), 0u);
    EXPECT_EQ(g.getNodes().count("n2"), 1u);

    // Edge store: e1 gone
    EXPECT_TRUE(g.getEdges().empty());

    // Adjacency: only n2 should be present, with empty lists
    ASSERT_EQ(g.getOutgoing().count("n2"), 1u);
    EXPECT_TRUE(g.getOutgoing().at("n2").empty());
    ASSERT_EQ(g.getIncoming().count("n2"), 1u);
    EXPECT_TRUE(g.getIncoming().at("n2").empty());
}

TEST(GraphUpdateNode, MergesAttributesAndEmitsEvent) {
    Graph g;
    // Setup: add a node with two attrs
    g.addNode("n1", {{"color","red"}, {"size","L"}}, /*ts=*/1);

    // Update: change size and add a new attr
    int64_t tsUpd = 2;
    g.updateNode("n1", {{"size","XL"}, {"status","active"}}, tsUpd);

    // We should have 2 events: ADD_NODE + UPDATE_NODE
    ASSERT_EQ(g.getEventLog().size(), 2u);
    const Event& ev = g.getEventLog().back();
    EXPECT_EQ(ev.type, EventType::UPDATE_NODE);
    EXPECT_EQ(ev.entityId, "n1");
    EXPECT_EQ(ev.timestamp, tsUpd);
    EXPECT_EQ(ev.payload.at("size"), "XL");
    EXPECT_EQ(ev.payload.at("status"), "active");

    // Node attrs should now be merged
    const Node& node = g.getNodes().at("n1");
    EXPECT_EQ(node.attributes.at("color"), "red");      // unchanged
    EXPECT_EQ(node.attributes.at("size"),  "XL");       // updated
    EXPECT_EQ(node.attributes.at("status"), "active");  // new
}

TEST(GraphUpdateEdge, MergesAttributesAndEmitsEvent) {
    Graph g;
    // Setup two nodes + edge
    g.addNode("n1", {}, /*ts=*/10);
    g.addNode("n2", {}, /*ts=*/20);
    g.addEdge("e1", "n1", "n2", {{"weight","5"}, {"type","undirected"}}, /*ts=*/30);

    // Update: change weight and add label
    int64_t tsUpd = 40;
    g.updateEdge("e1", {{"weight","10"}, {"label","flow"}}, tsUpd);

    // 4 total events
    ASSERT_EQ(g.getEventLog().size(), 4u);
    const Event& ev = g.getEventLog().back();
    EXPECT_EQ(ev.type, EventType::UPDATE_EDGE);
    EXPECT_EQ(ev.entityId, "e1");
    EXPECT_EQ(ev.timestamp, tsUpd);
    EXPECT_EQ(ev.payload.at("weight"), "10");
    EXPECT_EQ(ev.payload.at("label"),  "flow");

    // Edge attrs should now be merged
    const Edge& edge = g.getEdges().at("e1");
    EXPECT_EQ(edge.attributes.at("weight"), "10");   // updated
    EXPECT_EQ(edge.attributes.at("type"),   "undirected"); // unchanged
    EXPECT_EQ(edge.attributes.at("label"),  "flow");   // new

    // Adjacency must remain intact
    ASSERT_EQ(g.getOutgoing().at("n1").size(), 1u);
    EXPECT_EQ(g.getOutgoing().at("n1")[0], "e1");
    ASSERT_EQ(g.getIncoming().at("n2").size(), 1u);
    EXPECT_EQ(g.getIncoming().at("n2")[0], "e1");
}