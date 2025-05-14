# Graph Basics

ChronoGraph represents your data as an **event-sourced** directed graph: every mutation (add/delete/update) is recorded as an `Event`, and the live graph state is just the replay of those events.

*This tutorial goes over the basics of using graphs on their own. If you would like to use git-style commits, branching, merging, etc. you will need to create a Repository, which is essentially a wrapper around a graph.* Read more at [Repository Basics](repo_basics.md)

## 1. Creating a Graph

Include the core header and make an empty graph:

```cpp
#include <chronograph/graph/Graph.h>

chronograph::Graph g;
```

Nothing exists yet--no nodes or edges.

---

## 2. Mutating the Graph

All mutators take:
- a unique ID (`std::string`)
- a map of attributes (`std::map<std::string,std::string>`)
- a timestamp (`int64_t`)

### Add a node

```cpp
g.addNode("user1", {{"name","Alice"},{"role","admin"}}, /*ts=*/1);
``` 

### Add an edge

```cpp
g.addEdge("e1", "user1", "user2", {{"weight","5"}}, /*ts=*/2);
```

### Update Attributes

```cpp
g.updateNode("user1", {{"role","superadmin"}}, /*ts=*/3);
```

### Delete
```cpp
g.delEdge("e1", /*ts=*/4);
g.delNode("user2", /*ts=*/5);
```

---

## 3. Inspecting State & History

- **Live state**  
    ```cpp
    auto nodes = g.getNodes();     // map ID→Node
    auto edges = g.getEdges();     // map ID→Edge
    auto outgoing  = g.getOutgoing();  // map nodeID→[edgeIDs]
    ```

- **Event log**
    ```cpp
    const auto& log = g.getEventLog();
    for (auto& e : log) {
        // e.type, e.entityId, e.timestamp, e.payload, etc.
    }
    ```

---

## 4. Time-Travel with Snapshots

Reconstruct the graph as it stood at any timestamp:

```cpp
#include <chronograph/graph/Snapshot.h>

// build some events at ts=1,2,3…
auto snap = chronograph::Snapshot(g, /*timestamp=*/2);

// Now snap.getNodes(), snap.getEdges(), and snap.getOutgoing()
// reflect only events with ts ≤ 2.
```

## 5. Viewing Changes: `diff(t1,t2)`
Compute added/removed/updated nodes & edges between two times:

```cpp
auto d = g.diff(/*t1=*/1, /*t2=*/4);

// New nodes:
for (auto& n : d.nodesAdded) { /*…*/ }

// Removed edges:
for (auto& eid : d.edgesRemoved) { /*…*/ }

// Updated node attributes:
for (auto& [before, after] : d.nodesUpdated) { /*…*/ }
```

---

## 6. Quick Visualization: Graphviz DOT

Dump the live graph to a DOT string:

```cpp
#include <chronograph/graph/utils/Visual.h>

auto dot = chronograph::graph::algorithms::utils::toDot(g);
std::cout << dot;
```

Save it and render:

```cpp
echo "$dot" > graph.dot
dot -Tpng graph.dot -o graph.png
```
## 7. Next Steps
- Learn about **snapshots** in [Time-Travel & Snapshots](algorithms.md)  
- Explore **diffing**, **branches** & **merging** in the next tutorial  
- Try out **algorithms** (reachability, shortest-path) in [Algorithms](algorithms.md)