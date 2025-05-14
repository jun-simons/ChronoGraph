# Algorithms API

**Namespace:** `chronograph::graph::algorithms`  

A collection of common graph algorithms operating on the in-memory `Graph` or on snapshots. All functions take a `const Graph&` (or snapshot) and return results—no need to manipulate adjacency maps yourself.

## Reachability

Test whether `target` is reachable from `start` via any directed path.

```cpp
bool isReachable(
    const Graph& g,
    const std::string& start,
    const std::string& target
);
```

Returns `true` if there exists a path of edges from `start` to `target`.

Complexity: *O(N + E)* with BFS.

Time-travel version:
```cpp
bool isReachableAt(
    const Graph& g,
    const std::string& start,
    const std::string& target,
    std::int64_t timestamp
);
```
- Takes a snapshot at timestamp and runs reachability on that snapshot.

---

## Shortest Path

Compute an unweighted shortest (fewest‐hops) path.

```cpp
std::vector<std::string> shortestPath(
    const Graph& g,
    const std::string& start,
    const std::string& target
);
```
- Returns the node‐ID sequence `[start, …, target]`, or empty if no path.
- Complexity: *O(N + E)* with BFS + back-pointers.

---

## Time-Respecting Reachability

Only follow edges in non-decreasing creation timestamp order.

```cpp
bool isTimeRespectingReachable(
    const Graph& g,
    const std::string& start,
    const std::string& target
);
```
- Ensures each step moves along an edge whose createdTimestamp ≥ previous.
- Useful for temporal graph analyses.

---

## Connected Components

### Weakly-Connected

Treat directed edges as undirected:

```cpp
std::vector<std::vector<std::string>>
weaklyConnectedComponents(const Graph& g);
```
- Returns a list of components; each component is a vector of node IDs.
- Complexity: O(N + E) with BFS/Union-Find.

### Strongly-Connected
Find maximal directed cycles:
```cpp
std::vector<std::vector<std::string>>
stronglyConnectedComponents(const Graph& g);
```
- Uses Tarjan’s algorithm.
- **Each** returned component is a strongly connected set.

---

## Cycle Detection & Topological Sort

### hasCycle

Quick check for any directed cycle:

```cpp
bool hasCycle(const Graph& g);
```
- Equivalent to `topologicalSort(g).has_value() == false`.

### topologicalSort

Return an ordering if acyclic, else `std::nullopt`:

```cpp
std::optional<std::vector<std::string>>
topologicalSort(const Graph& g);
```

- Returns a vector of node IDs in topological order on success.
- Returns `nullopt` if the graph contains one or more cycles.
- Complexity: *O(N + E)* with Kahn’s algorithm.

---

## Example Usage

```cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/algorithms/Connectivity.h>

Graph g;
// … build your graph …

// 1) Simple reachability
bool canReach = graph::algorithms::isReachable(g,"A","B");

// 2) Shortest path
auto path = graph::algorithms::shortestPath(g,"A","C");

// 3) Weak CC
auto wcc = graph::algorithms::weaklyConnectedComponents(g);

// 4) Topo sort
if (auto topo = graph::algorithms::topologicalSort(g)) {
  for (auto& id : *topo) std::cout<<id<<" ";
} else {
  std::cerr<<"Cycle detected\n";
}
```

---

*For scenarios with snapshots, diffing, merges, see the tutorials under* `docs/tutorials/`.  
