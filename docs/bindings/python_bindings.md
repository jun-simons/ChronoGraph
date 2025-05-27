# Python Bindings (PyBind11)

ChronoGraph exposes its core C++ API to Python via PyBind11.  This document walks through how to build, install, and use the Python module.

---

## 1. Requirements

- **Python 3.8+** interpreter  
- **CMake 3.14+** with `add_subdirectory(extern/pybind11)` enabled  
- **PyBind11** (added as `extern/pybind11` submodule)  
- Your ChronoGraph C++ libraries already built

Before building, ensure:

```bash
git submodule update --init extern/pybind11
```

## 2. Building the Python Extension

1. In your top‐level `CMakeLists.txt` add:

```cmake
add_subdirectory(extern/pybind11)
add_subdirectory(bindings)
```

2. The `bindings/CMakeLists.txt` uses:

```cmake
pybind11_add_module(chronograph_module
  chronograph_py.cpp
)
set_target_properties(chronograph_module PROPERTIES
  OUTPUT_NAME "chronograph"
)
```

3. From your build directory:

```bash
mkdir -p build && cd build
cmake ..
make -j
```

## 3. Installing

To install into your active Python environment:

```bash
# from build directory
make install             # if CMAKE_INSTALL_PREFIX is set
# or manually copy:
cp bindings/chronograph.so /your/python/site-packages/
```

After installation, you can simply:

```python
import chronograph
```

in any Python script.

## 4. Bound Types & Modules

### `chronograph.Graph`

```python
g = chronograph.Graph()
g.add_node(id: str, attrs: dict[str,str], timestamp: int)
g.add_edge(id: str, from: str, to: str, attrs: dict[str,str], timestamp: int)
nodes = g.get_nodes()         # dict[str, Node]
edges = g.get_edges()         # dict[str, Edge]
out = g.get_outgoing()        # dict[str, list[str]]
```

### `chronograph.Node` & `chronograph.Edge`

```python
# Node supports dict‐style lookup into its .attributes:
n = nodes["A"]
assert n["role"] == "admin"   # forwards to n.attributes["role"]

# Edge fields:
e = edges["e1"]
print(e.from, e.to, e.attributes)
```

### `chronograph.Snapshot`


```python
# view graph at a past timestamp
snap = chronograph.Snapshot(g, timestamp=1234)
current_nodes = snap.get_nodes()
current_edges = snap.get_edges()
```


## 5. Algorithms
Exposed as free functions in the module:

```python
from chronograph import algorithms as alg

# reachability
alg.is_reachable(g, "A", "B")
alg.is_reachable_at(g, "A", "B", timestamp=42)
alg.is_time_respecting_reachable(g, "A", "B")

# shortest path
path = alg.shortest_path(g, "start", "end")

# connectivity & sorting
comps = alg.weakly_connected_components(g)
scc   = alg.strongly_connected_components(g)
cyc   = alg.has_cycle(g)
order = alg.topological_sort(g)  # returns list or None if cycle
```

## 6. Repository

```python
repo = chronograph.Repository.init("main")
repo.add_node("X", {}, ts=1)
cid1 = repo.commit("add X")

# branching
repo.branch("dev")
repo.checkout("dev")

# commit on dev
repo.add_node("Y", {}, ts=2)
cid2 = repo.commit("add Y")

# inspect working graph
nodes = repo.graph().get_nodes()
```

Merging:

```python
res = repo.merge("other_branch", policy=chronograph.MergePolicy.OURS)
conflicts = res.conflicts
```

## 7. Quick Example

```python
import chronograph
from chronograph import algorithms as alg

# build a simple graph
g = chronograph.Graph()
g.add_node("A", {}, 1)
g.add_node("B", {}, 2)
g.add_edge("e", "A", "B", {"w":"5"}, 3)

# snapshot
snap = chronograph.Snapshot(g, 2)
print(list(snap.get_nodes().keys()))  # ['A','B'] but no edges

# reachability
print(alg.is_reachable(g, "A","B"))  # True
```