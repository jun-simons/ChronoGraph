# ChronoGraph

**ChronoGraph** is a C++(>17) library for building and querying **temporal**, **versioned** graphs (“Git for graphs”).  You record every change as an append-only event log, and can:

- **Time-travel** to any past state via fast snapshot replay  
- **Branch** your graph history and **merge** divergent edits with three-way merge and conflict resolution  
- **Diff** two versions to see what nodes or edges were added, removed, or updated  

Chronograph can be used for social-network evolution, financial audit trails, collaborative knowledge-graph editing, and more.

---

## Features

- **Event model**: immutable `Event { id, timestamp, type, entityId, payload }`  
- **Core mutations**: `addNode`, `delNode`, `addEdge`, `delEdge`, `updateNode`, `updateEdge`  
- **In-memory state**: O(1) lookup of nodes/edges plus adjacency lists for fast traversal  
- **Snapshot**: reconstruct your graph at any timestamp by replaying events  
- *(Phase 2+)* `diff(t₁, t₂)`, commit/branch model, three-way merges with configurable conflict policies  
- *(Future)* will implement Python bindings via Pybind11 in `python/`

---

## Prerequisites

- A C++ compiler with **C++17** support (e.g. GCC 7+, Clang 5+, MSVC 2017+)  
- **CMake 3.14** or later  
- **Git** (to clone the repo)  
- **Internet access** (to fetch GoogleTest via CMake’s FetchContent)  

---

## Getting Started

```bash
# 1) Clone the repo
git clone https://github.com/yourusername/ChronoGraph.git
cd ChronoGraph

# 2) Create a build directory
mkdir build && cd build

# 3) Configure & build
cmake ..
make -j$(nproc)            # or `cmake --build . -- -j4` on some platforms

# 4) Run the unit tests
ctest --output-on-failure

# 5) (Optional) Try the example
./examples/basic_snapshot
```

This project is in early development 
(www.junsimons.com)