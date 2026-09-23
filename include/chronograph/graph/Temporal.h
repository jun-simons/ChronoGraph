// include/chronograph/graph/Temporal.h
#pragma once

#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace chronograph {

class Graph;

/// Queries over a graph's event history.
// * Results preserve event-log (causal) order; timestamps in the log are not
//   required to be sorted.
// * For the whole graph at a point in time, use Snapshot; these functions
//   answer questions about individual entities or spans of time without
//   materializing the full graph.
namespace temporal {

/// Inclusive time window [start, end]. Defaults to all of time.
struct TimeRange {
    std::int64_t start = std::numeric_limits<std::int64_t>::min();
    std::int64_t end   = std::numeric_limits<std::int64_t>::max();

    bool contains(std::int64_t t) const { return start <= t && t <= end; }

    /// Everything up to and including `t`
    static TimeRange until(std::int64_t t) {
        return {std::numeric_limits<std::int64_t>::min(), t};
    }
};

/// Every event whose timestamp falls within `range`.
std::vector<Event> eventsInRange(const Graph& g, const TimeRange& range);

/// Events for node `id` (ADD_NODE / UPDATE_NODE / DEL_NODE) within `range`.
std::vector<Event> nodeHistory(const Graph& g, const std::string& id,
                               const TimeRange& range = {});

/// Events for edge `id` (ADD_EDGE / UPDATE_EDGE / DEL_EDGE) within `range`.
/// Includes deletions cascaded from removing an endpoint node.
std::vector<Event> edgeHistory(const Graph& g, const std::string& id,
                               const TimeRange& range = {});

/// Node `id` as it was at `timestamp`, or nullopt if it didn't exist then.
std::optional<Node> nodeAt(const Graph& g, const std::string& id,
                           std::int64_t timestamp);

/// Edge `id` as it was at `timestamp`, or nullopt if it didn't exist then.
std::optional<Edge> edgeAt(const Graph& g, const std::string& id,
                           std::int64_t timestamp);

/// Distinct event timestamps in ascending order: every moment at which the
/// graph changed. Useful for iterating Snapshots over a graph's lifetime.
std::vector<std::int64_t> changeTimes(const Graph& g, const TimeRange& range = {});

}  // namespace temporal
}  // namespace chronograph
