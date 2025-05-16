# Algorithms Basics

ChronoGraph bundles a set of graph algorithms under `chronograph::graph::algorithms`. To use it, you simply pass your `Graph` (or `Snapshot`) and get back results.

## 1. Reachability

Test whether there is **any** path from `start` to `target`:

```cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>

chronograph::Graph g;
// … build nodes & edges …
bool ok = chronograph::graph::algorithms::isReachable(g, "A", "D");
if (ok) std::cout<<"A can reach D\n";
```

---

## 2. Shortest‐Path (Unweighted)

BFS‐based minimum‐hop path:

```cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>

auto path = chronograph::graph::algorithms::shortestPath(g, "A", "D");
if (!path.empty()) {
  std::cout<<"Path:";
  for (auto& node : path) std::cout<<" "<<node;
  std::cout<<"\n";
}
```

---

## 3. Time‐Travel Reachability

Check if `target` is reachable from `start` **as of** timestamp T?”:

```cpp
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/algorithms/Paths.h>

bool wasReachable = chronograph::graph::algorithms::isReachableAt(
    g, "X", "Y", /*timestamp=*/42);
```

---

## 4. Time‐Respecting Reachability

Only follow edges in non-decreasing timestamp order:

```cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/algorithms/Paths.h>

bool ok = chronograph::graph::algorithms::isTimeRespectingReachable(
    g, "A", "B");
```


---

## 5. Connected Components

- **Weakly Connected** (treat directed edges as undirected)  
- **Strongly Connected** (maximal cycles)

```cpp
#include <chronograph/graph/algorithms/Connectivity.h>

// Weakly
auto wcc = chronograph::graph::algorithms::weaklyConnectedComponents(g);

// Strongly
auto scc = chronograph::graph::algorithms::stronglyConnectedComponents(g);
```


---

## 6. Cycle Detection & Topological Sort

- **hasCycle(g)** returns `true` if any directed cycle exists.  
- **topologicalSort(g)** returns an ordered list or `nullopt` on cycle.

```cpp
#include <chronograph/graph/algorithms/Connectivity.h>

if (chronograph::graph::algorithms::hasCycle(g)) {
  std::cerr<<"Graph has a cycle\n";
} else {
  auto optOrder = chronograph::graph::algorithms::topologicalSort(g);
  for (auto& id : *optOrder) std::cout<<id<<" ";
}
```

---


## Next Steps

- Explore **[Repository Basics](repository_basics.md)** to version your graph  
- See **[Examples](examples.md)** for complete sample programs  
