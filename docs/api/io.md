# Saving & Loading API

**Header:** `include/chronograph/io/Serialization.h`  
**Namespace:** `chronograph::io`  
**Library target:** `chronograph-io` (included in `chronograph`)

Graphs and repositories are saved as versioned JSON documents. Only history is stored; the live state is rebuilt on load:

- **Graph:** its event log.
- **Repository:** all commits, branch pointers, the checked-out branch, and any **uncommitted** events, so nothing staged is lost.

For a given history the output is deterministic (stable key and commit order), so saved files diff cleanly under version control.

## Functions

```cpp
// Files
void       saveGraph(const Graph& graph, const std::filesystem::path& path);
Graph      loadGraph(const std::filesystem::path& path);
void       saveRepository(const Repository& repo, const std::filesystem::path& path);
Repository loadRepository(const std::filesystem::path& path);

// JSON text
std::string graphToJson(const Graph& graph);
Graph       graphFromJson(const std::string& json);
std::string repositoryToJson(const Repository& repo);
Repository  repositoryFromJson(const std::string& json);
```

- Saves write to `<path>.tmp` and then rename it over `path`, so an existing file is never left half-written.
- **Errors:**
  - `io::FormatError` (a `std::runtime_error`) means the contents are wrong: malformed JSON, missing fields, the wrong document kind, an unsupported version, or an inconsistent history (unknown parents, dangling branches, …).
  - A plain `std::runtime_error` means a file-system problem (can't open, can't write).

```cpp
#include <chronograph/io/Serialization.h>
using namespace chronograph;

io::saveRepository(repo, "graph-repo.json");
Repository restored = io::loadRepository("graph-repo.json");
```

## Format (version 1)

```json
{
  "format": "chronograph.repository",
  "version": 1,
  "head": "main",
  "branches": { "main": "f08d590ba976c686" },
  "commits": [
    { "id": "5b6c...", "parents": [], "message": "", "events": [] },
    { "id": "f08d...", "parents": ["5b6c..."], "message": "first", "events": [
        { "id": "62c6...", "type": "ADD_NODE", "entity": "A", "timestamp": 1,
          "payload": { "name": "Alice" } },
        { "id": "9006...", "type": "ADD_EDGE", "entity": "e", "from": "A", "to": "B",
          "timestamp": 3 }
    ] }
  ],
  "staged": []
}
```

- Commits are listed parents-first, and a repository has exactly one root commit.
- A graph document is `{"format": "chronograph.graph", "version": 1, "events": [...]}`.
- Empty `payload`, `from` and `to` fields are omitted.
- Loaders accept any version up to `io::kFormatVersion`.

## Lower-level: `RepositoryData`

The io layer never touches `Repository` internals. It goes through a plain data struct, which you can also use to build other storage formats:

```cpp
RepositoryData data = repo.exportData();          // commits, branches, head, staged
Repository copy     = Repository::fromData(data); // validates; throws std::invalid_argument
```

## Python

```python
chronograph.save_repository(repo, "repo.json")     # str or pathlib.Path
repo = chronograph.load_repository("repo.json")
chronograph.save_graph(g, path); g = chronograph.load_graph(path)
text = chronograph.repository_to_json(repo)

try:
    chronograph.load_graph("bad.json")
except chronograph.FormatError:                    # subclass of ValueError
    ...
```
