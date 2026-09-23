// tests/test_Temporal.cpp

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/Temporal.h>
#include <chronograph/graph/algorithms/Paths.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace chronograph;
using temporal::TimeRange;

namespace {

// n: added @1, updated @3, deleted @5 (cascading e), re-added @7
// m: added @2;  e: n->m added @4
Graph lifecycleGraph() {
    Graph g;
    g.addNode("n", {{"v","1"}}, 1);
    g.addNode("m", {}, 2);
    g.updateNode("n", {{"v","2"}, {"w","x"}}, 3);
    g.addEdge("e", "n", "m", {{"k","v"}}, 4);
    g.delNode("n", 5);
    g.addNode("n", {{"v","fresh"}}, 7);
    return g;
}

std::vector<EventType> types(const std::vector<Event>& events) {
    std::vector<EventType> out;
    for (const auto& e : events) out.push_back(e.type);
    return out;
}

}  // namespace

TEST(Temporal, EntityHistoryAndRanges) {
    Graph g = lifecycleGraph();

    EXPECT_EQ(types(temporal::nodeHistory(g, "n")),
              (std::vector<EventType>{EventType::ADD_NODE, EventType::UPDATE_NODE,
                                      EventType::DEL_NODE, EventType::ADD_NODE}));
    // Inclusive window, and only this node's events (not e's cascade)
    EXPECT_EQ(types(temporal::nodeHistory(g, "n", {3, 5})),
              (std::vector<EventType>{EventType::UPDATE_NODE, EventType::DEL_NODE}));
    // Cascaded deletion shows up in the edge's own history
    EXPECT_EQ(types(temporal::edgeHistory(g, "e")),
              (std::vector<EventType>{EventType::ADD_EDGE, EventType::DEL_EDGE}));
    EXPECT_TRUE(temporal::nodeHistory(g, "missing").empty());

    // DEL_EDGE and DEL_NODE both happen @5
    EXPECT_EQ(temporal::eventsInRange(g, {4, 5}).size(), 3u);
    EXPECT_EQ(temporal::changeTimes(g), (std::vector<std::int64_t>{1, 2, 3, 4, 5, 7}));
    EXPECT_EQ(temporal::changeTimes(g, TimeRange::until(3)),
              (std::vector<std::int64_t>{1, 2, 3}));
}

TEST(Temporal, PointInTimeLookups) {
    Graph g = lifecycleGraph();

    EXPECT_FALSE(temporal::nodeAt(g, "n", 0).has_value());
    EXPECT_EQ(temporal::nodeAt(g, "n", 1)->attributes.at("v"), "1");
    auto n3 = temporal::nodeAt(g, "n", 4);
    ASSERT_TRUE(n3.has_value());
    EXPECT_EQ(n3->attributes, (std::map<std::string, std::string>{{"v","2"}, {"w","x"}}));
    EXPECT_FALSE(temporal::nodeAt(g, "n", 6).has_value());
    // Re-added node starts from its new attributes only
    EXPECT_EQ(temporal::nodeAt(g, "n", 7)->attributes,
              (std::map<std::string, std::string>{{"v","fresh"}}));

    EXPECT_FALSE(temporal::edgeAt(g, "e", 3).has_value());
    auto e = temporal::edgeAt(g, "e", 4);
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->from, "n");
    EXPECT_EQ(e->createdTimestamp, 4);
    EXPECT_FALSE(temporal::edgeAt(g, "e", 5).has_value());

    // Agrees with a full Snapshot
    Snapshot s(g, 4);
    EXPECT_EQ(*temporal::nodeAt(g, "n", 4), s.getNodes().at("n"));
    EXPECT_EQ(*e, s.getEdges().at("e"));
}

TEST(Temporal, AlgorithmsAndDiffOnSnapshots) {
    Graph g = lifecycleGraph();
    using graph::algorithms::isReachable;

    // Algorithms run on any GraphView: the past vs. the present
    EXPECT_TRUE(isReachable(Snapshot(g, 4), "n", "m"));
    EXPECT_FALSE(isReachable(g, "n", "m"));

    auto d = diff(Snapshot(g, 4), g);
    EXPECT_EQ(d.edgesRemoved, (std::vector<std::string>{"e"}));
    ASSERT_EQ(d.nodesUpdated.size(), 1u);  // n was deleted and re-added
    EXPECT_EQ(d.nodesUpdated[0].second.attributes.at("v"), "fresh");
    EXPECT_TRUE(diff(g, g).empty());
}
