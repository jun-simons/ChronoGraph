# ChronoGraph

*Github, but for graphs!*

A temporal, versioned graph library in C++17 with Git-like branching & merging, time-travel snapshots, diffs, and graph algorithms.

Chronograph can be used for social-network evolution, financial audit trails, collaborative knowledge-graph editing, and more.

---

## Features

- **Event Sourcing**  
  Append-only history of node/edge additions, deletions, and updates with timestamps.

- **Snapshots & Time-Travel**  
  View the graph at a certain point in time, or *replay* the graph changes between two points in time.

- **Diffing**  
  Compute added/removed/updated nodes and edges between two points in time, or between two branches

- **Repository & Commits**  
  ChronoGraph provides a Respoitory object at the top level. Changes to the graph are committed and stored as `Commit` objects.

- **Branching & Merging**  
  Graph repositories support branches, fast-forward and three-way merges (with pluggable merge policies).

- **Graph Algorithms**  
  Reachability, shortest-path stubs, and more, via a dedicated `graph/algorithms/` submodule.

## Prerequisites

You need:
- *CMake 3.14* or later
- A C++17-capable compiler (e.g. GCC 7+, Clang 5+)
- GoogleTest (or internet access: by default, GTest is fetched by CMake's FetchContent)

## Getting Started

```bash
# 1) Clone the repo
git clone https://github.com/jun-simons/ChronoGraph.git
cd ChronoGraph

# 2) Create a build directory
mkdir build && cd build

# 3) Configure & build
cmake ..
make -j$(nproc)            # or `cmake --build . -- -j4` on some platforms

# 4) (Optional) Run the unit tests
ctest --output-on-failure -V

# 5) (Optional) Run the example code
./examples/example_repo
```

---

#### Basic Usage: Graph

```cpp
#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>

using namespace chronograph;

Graph g;
g.addNode("n1", {{"name","Alice"}}, timestamp);
g.addNode("n2", {{"name","Bob"}}, timestamp);
g.addEdge("e1","n1","n2",{},timestamp);

Snapshot snap(g, someTimestamp);
auto nodes = snap.getNodes();
```

---

#### Basic Usage: Reachability

```cpp
#include <chronograph/graph/algorithms/Paths.h>

using chronograph::graph::algorithms::isReachable;

bool canReach = isReachable(g, "A", "B");
```

---

#### Basic Usage: Repository (Commits & Branches)

```cpp
#include <chronograph/repo/Repository.h>

auto repo = Repository::init("main");
repo.addNode("A", {...}, ts);
auto c1 = repo.commit("add A");

repo.branch("dev");
repo.checkout("dev");
repo.addNode("B", {...}, ts);
auto c2 = repo.commit("add B");

repo.checkout("main");
auto result = repo.merge("dev", MergePolicy::OURS);
```

## Documentation

For in-depth design and API details, see the `docs/` folder.

## License

MIT © Jun Simons


This project is in early development 
(www.junsimons.com)

