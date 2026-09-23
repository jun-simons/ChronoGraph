// src/Temporal.cpp
#include <chronograph/graph/Temporal.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/GraphState.h>
#include <algorithm>

namespace chronograph {
namespace temporal {

namespace {
    bool isNodeEvent(EventType t) {
        return t == EventType::ADD_NODE || t == EventType::UPDATE_NODE ||
               t == EventType::DEL_NODE;
    }
    bool isEdgeEvent(EventType t) {
        return t == EventType::ADD_EDGE || t == EventType::UPDATE_EDGE ||
               t == EventType::DEL_EDGE;
    }

    template <typename Pred>
    std::vector<Event> filterLog(const Graph& g, Pred keep) {
        std::vector<Event> out;
        for (const auto& e : g.getEventLog()) {
            if (keep(e)) out.push_back(e);
        }
        return out;
    }

    // Replay one entity's events into a scratch state, so
    // point-in-time lookups follow exactly the same rules as Snapshot
    GraphState replayEntity(const std::vector<Event>& history) {
        GraphState state;
        for (const auto& e : history) state.apply(e);
        return state;
    }
} // anonymous

std::vector<Event> eventsInRange(const Graph& g, const TimeRange& range) {
    return filterLog(g, [&](const Event& e) { return range.contains(e.timestamp); });
}

std::vector<Event> nodeHistory(const Graph& g, const std::string& id,
                               const TimeRange& range) {
    return filterLog(g, [&](const Event& e) {
        return isNodeEvent(e.type) && e.entityId == id && range.contains(e.timestamp);
    });
}

std::vector<Event> edgeHistory(const Graph& g, const std::string& id,
                               const TimeRange& range) {
    return filterLog(g, [&](const Event& e) {
        return isEdgeEvent(e.type) && e.entityId == id && range.contains(e.timestamp);
    });
}

std::optional<Node> nodeAt(const Graph& g, const std::string& id,
                           std::int64_t timestamp) {
    auto state = replayEntity(nodeHistory(g, id, TimeRange::until(timestamp)));
    auto it = state.nodes.find(id);
    if (it == state.nodes.end()) return std::nullopt;
    return it->second;
}

std::optional<Edge> edgeAt(const Graph& g, const std::string& id,
                           std::int64_t timestamp) {
    auto state = replayEntity(edgeHistory(g, id, TimeRange::until(timestamp)));
    auto it = state.edges.find(id);
    if (it == state.edges.end()) return std::nullopt;
    return it->second;
}

std::vector<std::int64_t> changeTimes(const Graph& g, const TimeRange& range) {
    std::vector<std::int64_t> times;
    for (const auto& e : g.getEventLog()) {
        if (range.contains(e.timestamp)) times.push_back(e.timestamp);
    }
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    return times;
}

}  // namespace temporal
}  // namespace chronograph
