# Core Graph API

**Header:** `include/chronograph/graph/Graph.h`  

The `Graph` class provides event‐sourced mutators for nodes & edges, access to the live state and history, plus utility operations (snapshots, diffs, checkpoints).

## 1. Append‐Only Events

### `void addEvent(const Event& e);`

- **Description:**  
  Append a raw `Event` to the internal history without updating any state.  
- **Use Case:**  
  Low-level replay or applying events from another source.  
- **Parameters:**  
  - `e` – fully populated `Event` struct.  
- **Returns:**  
  - _(void)_

```cpp
void addEvent(const Event& e);
```

## 2. Mutators

All mutators append a corresponding `Event` **and** update the live graph state immediately.

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
    std::int64_t timestamp; 
    size_t       eventIndex; 
    std::unordered_map<std::string,Node> nodes; 
    std::unordered_map<std::string,Edge> edges; 
    std::unordered_map<std::string,std::vector<std::string>> outgoing, incoming;
};

const std::vector<Checkpoint>& getCheckpoints() const;
```
- **Purpose:** Speed up snapshot construction.  
- **`getCheckpoints()`** returns periodically‐saved states (every N events).

## 4. Utilities

### Applying & Clearing State

```cpp
void applyEvent(const Event& e);
void clearStateKeepLog();
void clearGraph();
```
- `applyEvent(e)` – update live state from event `e` without logging.
- `clearStateKeepLog()` – wipe state but keep `eventLog_` (for replay).
- `clearGraph()` – wipe both state and history.


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

Compute changes between two timestamps:

- **Returns** a `DiffResult` grouping added/removed/updated nodes and edges.  
- **Usage:**  
```cpp
auto d = g.diff(100, 200);
for (auto& n : d.nodesAdded) { /* … */ }
```

---

*End of Graph API reference.*  
