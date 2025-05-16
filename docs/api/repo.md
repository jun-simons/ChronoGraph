# Repository API

**Header:** `include/chronograph/repo/Repository.h`  

Git-style versioning for your graph. Staging area, commits, branches, checkouts, and merges—all in one class.

## 1. Commit Data Types

### `struct Commit`

```cpp
struct Commit {
    std::string               id;       // unique commit hash/UUID
    std::vector<std::string>  parents;  // parent commit IDs (1 or 2 for merges)
    std::vector<Event>        events;   // delta of events since parent
    std::string               message;  // commit message
};
```

- `id`: identifier for this commit
- `parents`: DAG edges to previous commit(s)
- `events`: the list of Event objects recorded in this commit
- `message`: human-readable description

---

```markdown
### `struct CommitGraph`

```cpp
struct CommitGraph {
  std::vector<std::string>                                        commitIds;
  std::unordered_map<std::string,std::vector<std::string>>        parents;
  std::unordered_map<std::string,std::vector<std::string>>        children;
};
```
- `commitIds`: all commits in the repo
- `parents[cid]`: list of parent IDs for commit `cid`
- `children[cid]`: list of child IDs for commit `cid`


---

```markdown
## 2. Merge & Conflict Types

### `enum class MergePolicy`

```cpp
enum class MergePolicy { OURS, THEIRS, ATTRIBUTE_UNION, INTERACTIVE };
- `OURS`: prefer current branch on conflicts
- `THEIRS`: prefer incoming branch
- `ATTRIBUTE_UNION`: merge attribute maps
- `INTERACTIVE`: collect conflicts for manual resolution
```

## 3. The `Repository` Class

Provides a versioned overlay on top of `Graph`.

```cpp
class Repository {
public:
    static Repository init(const std::string& rootBranch = "main");
    // staging mutators…
    // commit, branch, checkout, listBranches, listCommits, getCommitGraph, merge
    const Graph& graph() const;
};
```


---

### `init(rootBranch)`

```cpp
static Repository init(const std::string& rootBranch = "main");
```

- **Description:** Create a new repository with one initial “root” commit on `rootBranch`.  
- **Parameters:**  
  - `rootBranch` – name of the first branch (default `"main"`)  
- **Returns:** A `Repository` instance ready for staging. 

### Staging Mutators

```cpp
void addNode(const std::string& id,
             const std::map<std::string,std::string>& attrs,
             std::int64_t timestamp);
void delNode(const std::string& id, std::int64_t timestamp);
void addEdge(const std::string& id,
             const std::string& from,
             const std::string& to,
             const std::map<std::string,std::string>& attrs,
             std::int64_t timestamp);
void delEdge(const std::string& id, std::int64_t timestamp);
void updateNode(const std::string& id,
                const std::map<std::string,std::string>& attrs,
                std::int64_t timestamp);
void updateEdge(const std::string& id,
                const std::map<std::string,std::string>& attrs,
                std::int64_t timestamp);
```
- Mirror `Graph`’s mutators, but stage into the working-tree.  
- All changes affect `repo.graph()` and are recorded for the next commit.  
- **Parameters** match `Graph` methods exactly (ID, attrs, ts).  