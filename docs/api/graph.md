# Core Graph API

**Header:** `include/chronograph/graph/Graph.h`  

`Graph` derives from [`GraphView`](snapshot.md), which supplies the read accessors (`getNodes`, `getEdges`, `getOutgoing`, `getIncoming`, `hasNode`, `hasEdge`).

The `Graph` class provides event‐sourced mutators for nodes & edges, access to the live state and history, plus utility operations (snapshots, diffs, checkpoints).

## 1. Append‐Only Events

### `void addEvent(const Event& e);`

- **Description:**  
  Append a raw `Event` to the internal history and apply it to the live state. No validation is performed.  
- **Use Case:**  
  Low-level replay or applying events from another source (used by `Repository` on checkout/merge).  
- **Parameters:**  
  - `e` – fully populated `Event` struct.  
- **Returns:**  
  - _(void)_

```cpp
void addEvent(const Event& e);
```

## 2. Mutators

All mutators append a corresponding `Event` **and** update the live graph state immediately.

Mutators validate their input and throw `std::invalid_argument` (Python: `ValueError`) — leaving the graph unchanged — when:
- adding a node or edge whose ID already exists,
- adding an edge whose `from` or `to` node doesn't exist,
- deleting or updating a node/edge that doesn't exist.

`delNode` records a `DEL_EDGE` event for every incident edge **before** its `DEL_NODE` event.

---

### `void addNode(...)`

```cpp
void addNode(
    const std::string& id,
    const std::map<std::string,std::string>& attrs,
    std::int64_t timestamp
);
```
- **Description:**  
  Add a new node with `id` and initial attributes at `timestamp`.
- **Use Case:**  
  Low-level replay or applying events from another source.  
- **Parameters:**  
  - `id` – unique node identifier
  - `attrs` – map of attribute key/value pairs
  - `timestamp` – event time (monotonic)
- **Behavior:**  
  - Emits an `ADD_NODE` event.
  - Inserts into `nodes_`, creates empty adjacency lists.

---

### `void delNode(const std::string& id, std::int64_t timestamp)`

```cpp
void delNode(const std::string& id, std::int64_t timestamp);
```

- **Description:**  
  Delete the node `id` at `timestamp`.  
- **Parameters:**  
  - `id`        – node to remove  
  - `timestamp` – event time  
- **Behavior:**  
  - Emits a `DEL_NODE` event.  
  - Removes from `nodes_`, prunes any attached edges and adjacency entries.

---

### `void updateNode(...)`

```cpp
  void updateNode(
    const std::string& id,
    const std::map<std::string,std::string>& attrs,
    std::int64_t timestamp
);
```
- **Description:**  
  Overwrite the attributes of existing node `id` at `timestamp`.  
- **Parameters:**  
  - `id`        – node to update  
  - `attrs`     – new attribute map  
  - `timestamp` – event time  
- **Behavior:**  
  - Emits an `UPDATE_NODE` event.  
  - Replaces the `attributes` map in `nodes_[id]`.

---

### `void addEdge(...)`

```cpp
void addEdge(
    const std::string& id,
    const std::string& from,
    const std::string& to,
    const std::map<std::string,std::string>& attrs,
    std::int64_t timestamp
);
```

- **Description:**  
  Add a directed edge `id` from node `from` to node `to`.  
- **Parameters:**  
  - `id`        – unique edge identifier  
  - `from`      – source node ID  
  - `to`        – target node ID  
  - `attrs`     – edge attributes  
  - `timestamp` – event time  
- **Behavior:**  
  - Emits `ADD_EDGE` event.  
  - Inserts into `edges_`, updates `outgoing_[from]` and `incoming_[to]`.

---

### `void delEdge(...)`

```cpp
void delEdge(const std::string& id, std::int64_t timestamp);
```

- **Description:**  
  Delete edge `id` at `timestamp`.  
- **Parameters:**  
  - `id`        – edge to remove  
  - `timestamp` – event time  
- **Behavior:**  
  - Emits `DEL_EDGE` event.  
  - Removes from `edges_`, erases from adjacency lists.

---

### `void updateEdge(...)`

```cpp
void updateEdge(
    const std::string& id,
    const std::map<std::string,std::string>& attrs,
    std::int64_t timestamp
);
```

- **Description:**  
  Overwrite attributes of existing edge `id`.  
- **Parameters:**  
  - `id`        – edge to update  
  - `attrs`     – new attribute map  
  - `timestamp` – event time  
- **Behavior:**  
  - Emits `UPDATE_EDGE` event.  
  - Replaces `attributes` map in `edges_[id]`.

## 3. Inspectors & Accessors

### Live State

```cpp
const std::unordered_map<std::string,Node>& getNodes()   const;
const std::unordered_map<std::string,Edge>& getEdges()   const;
const std::unordered_map<std::string,std::vector<std::string>>& getOutgoing() const;
const std::unordered_map<std::string,std::vector<std::string>>& getIncoming() const;
```
- Description:
Read-only views of current nodes, edges, and adjacency.

---

### Event History

```cpp
const std::vector<Event>& getEventLog() const;
```

- **`getEventLog()`** returns the full append-only event sequence (`EventType`, `entityId`, `timestamp`, etc.).

---


### Checkpoints

```cpp
struct Checkpoint {
    std::int64_t timestamp;   // latest timestamp among the events it covers
    size_t       eventIndex;  // number of events folded into `state`
    GraphState   state;       // nodes, edges, outgoing, incoming
};

const std::vector<Checkpoint>& getCheckpoints() const;
```
- **Purpose:** Speed up snapshot construction.  
- **`getCheckpoints()`** returns periodically‐saved states (every N events). A snapshot at time `T` starts from the latest checkpoint whose `timestamp <= T`.

## 4. Utilities

### Clearing State

```cpp
void clearGraph();
```
- `clearGraph()` – wipe state, history and checkpoints.


---

### Diff

```cpp
struct DiffResult {
  std::vector<Node>                 nodesAdded;
  std::vector<std::string>          nodesRemoved;
  std::vector<std::pair<Node,Node>> nodesUpdated;
  std::vector<Edge>                 edgesAdded;
  std::vector<std::string>          edgesRemoved;
  std::vector<std::pair<Edge,Edge>> edgesUpdated;
};

DiffResult diff(std::int64_t t1, std::int64_t t2) const;
```

Compute changes between two timestamps (shorthand for `diff(Snapshot(g, t1), Snapshot(g, t2))`; see [Snapshots, Views & Diffs](snapshot.md)):

- **Returns** a `DiffResult` grouping added/removed/updated nodes and edges.  
- **Usage:**  
```cpp
auto d = g.diff(100, 200);
for (auto& n : d.nodesAdded) { /* … */ }
```

---

*End of Graph API reference.*  
