# Snapshots, Views & Diffs

**Headers:** `include/chronograph/graph/Snapshot.h`, `GraphView.h`, `Diff.h`

## `GraphView`

The read-only interface shared by `Graph` (live state) and `Snapshot` (state at a point in time). Algorithms, diffs and `toDot` take a `const GraphView&`, so they work on either.

```cpp
const std::unordered_map<std::string, Node>& getNodes() const;
const std::unordered_map<std::string, Edge>& getEdges() const;
const std::unordered_map<std::string, std::vector<std::string>>& getOutgoing() const; // node -> edge IDs
const std::unordered_map<std::string, std::vector<std::string>>& getIncoming() const; // node -> edge IDs
bool hasNode(const std::string& id) const;
bool hasEdge(const std::string& id) const;
```

`GraphView` can't be constructed or destroyed on its own; it only exists as part of a `Graph` or `Snapshot`.

## `Snapshot`

```cpp
Snapshot(const Graph& graph, std::int64_t timestamp);
```

- **Description:** Rebuilds the graph as it was at `timestamp` by replaying every event with a timestamp `<= timestamp`, starting from the latest usable checkpoint.
- The event log doesn't have to be sorted by time (merges append older events); a snapshot includes every event at or before `timestamp`, applied in log order.
- A `Snapshot` is an independent copy: later changes to the graph don't affect it.

```cpp
using namespace chronograph;
using graph::algorithms::isReachable;

Snapshot past(g, 100);
bool then = isReachable(past, "A", "B");   // algorithms accept snapshots
bool now  = isReachable(g, "A", "B");
```

## Diffs

```cpp
struct DiffResult {
    std::vector<Node>                 nodesAdded;
    std::vector<std::string>          nodesRemoved;
    std::vector<std::pair<Node,Node>> nodesUpdated;   // {before, after}
    std::vector<Edge>                 edgesAdded;
    std::vector<std::string>          edgesRemoved;
    std::vector<std::pair<Edge,Edge>> edgesUpdated;   // {before, after}
    bool empty() const;
};

DiffResult diff(const GraphView& before, const GraphView& after);
```

- Compares any two views: two snapshots, a snapshot and the live graph, or graphs from different branches.
- `Graph::diff(t1, t2)` is shorthand for `diff(Snapshot(g, t1), Snapshot(g, t2))`.
- `Repository::diff(fromRef, toRef)` compares two commits or branches (see [Repository](repo.md)).
