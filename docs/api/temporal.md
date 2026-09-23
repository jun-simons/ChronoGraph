# Temporal Queries API

**Header:** `include/chronograph/graph/Temporal.h`  
**Namespace:** `chronograph::temporal`

Questions about how individual entities, or the graph as a whole, changed over time. These read the event log directly; use a [`Snapshot`](snapshot.md) when you need the whole graph at one moment.

Results keep event-log (causal) order. Timestamps in the log don't have to be sorted.

## `TimeRange`

```cpp
struct TimeRange {
    std::int64_t start = INT64_MIN;
    std::int64_t end   = INT64_MAX;
    bool contains(std::int64_t t) const;      // start <= t && t <= end
    static TimeRange until(std::int64_t t);   // [INT64_MIN, t]
};
```

An **inclusive** window. The default covers all of time.

## Entity history

```cpp
std::vector<Event> nodeHistory(const Graph& g, const std::string& id, const TimeRange& range = {});
std::vector<Event> edgeHistory(const Graph& g, const std::string& id, const TimeRange& range = {});
```

- `nodeHistory` returns the node's `ADD_NODE` / `UPDATE_NODE` / `DEL_NODE` events.
- `edgeHistory` returns the edge's `ADD_EDGE` / `UPDATE_EDGE` / `DEL_EDGE` events, including deletions caused by removing an endpoint node.
- A node and an edge may share an ID; each function only returns events for its own entity kind.

## Point-in-time lookups

```cpp
std::optional<Node> nodeAt(const Graph& g, const std::string& id, std::int64_t timestamp);
std::optional<Edge> edgeAt(const Graph& g, const std::string& id, std::int64_t timestamp);
```

- Return the entity as it was at `timestamp`, or `std::nullopt` if it didn't exist then.
- They replay only that entity's events, so they're cheaper than a full snapshot and give the same answer.

## Spans of time

```cpp
std::vector<Event>        eventsInRange(const Graph& g, const TimeRange& range);
std::vector<std::int64_t> changeTimes(const Graph& g, const TimeRange& range = {});
```

- `eventsInRange` returns every event within the window.
- `changeTimes` returns the distinct timestamps at which the graph changed, in ascending order. It's handy for walking a graph's lifetime:

```cpp
for (auto t : temporal::changeTimes(g)) {
    Snapshot s(g, t);
    std::cout << t << ": " << weaklyConnectedComponents(s).size() << " components\n";
}
```

## Python

```python
from chronograph import temporal

temporal.node_history(g, "A")                      # list[Event]
temporal.node_history(g, "A", temporal.TimeRange(start=10, end=20))
temporal.node_at(g, "A", 15)                       # Node or None
temporal.change_times(g, temporal.TimeRange(end=100))
```
