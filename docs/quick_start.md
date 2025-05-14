# Quick Start

In just a few lines of C++ you can create a temporal graph, insert nodes and edges, take a time‐travel snapshot, and inspect the state. Let’s walk through a minimal example.

*Note: This only demonstrates the temporal graphs, but not the repository handler.*  Read about repositories at [Repository Basics](tutorials/repo_basics.md)

## Example: Simple Graph + Snapshot

Create a file `examples/quick_start.cpp`:

```cpp
#include <iostream>
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>

int main() {
    // 1) Build the graph
    chronograph::Graph g;
    int64_t ts = 1;
    g.addNode("Alice", {{"role","admin"}}, ts++);
    g.addNode("Bob",   {{"role","user"}},  ts++);
    g.addEdge("e1", "Alice", "Bob", {},     ts++);

    // 2) Take a snapshot at the current timestamp
    auto snap = chronograph::Snapshot(g, ts);

    // 3) Print each node and its attributes
    for (auto& [id, node] : snap.getNodes()) {
        std::cout << id << ": ";
        for (auto& [k, v] : node.attributes) {
            std::cout << k << "=" << v << " ";
        }
        std::cout << "\n";
    }
    return 0;
}
```

---

## Build & Run

```bash
# From your project root
mkdir -p build && cd build
cmake ..
make -j   # or `cmake --build . -- -j4`

# Compile the quick_start example
g++ ../examples/quick_start.cpp -I../include -L. -lchronograph-graph -lchronograph-graph-utils -std=c++17 -o quick_start

# Run it
./quick_start
```

---

## Next Steps

- View [Graph Basics](tutorials/graph_basics.md) to learn about events, deletions, and updates  
- View [Repository Basics](tutorials/repository_basics.md) for Git-style branching & merging  
- Try out algorithms like reachability and shortest-path in [Algorithms Tutorial](tutorials/algorithms.md)
