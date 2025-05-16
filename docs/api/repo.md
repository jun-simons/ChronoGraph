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


### `commit(message)`
```cpp
std::string commit(const std::string& message = "");
```
- **Description:** Record all staged events since the last commit into a new `Commit`.  
- **Parameters:**  
  - `message` – commit message (default empty)  
- **Returns:** New commit `id`.  
- **Effects:** Advances the current branch’s tip to this new commit.  

### `branch(branchName)`

```cpp
void branch(const std::string& branchName);
```

- **Description:** Create a new branch pointer at the current `HEAD` commit.  
- **Parameters:**  
  - `branchName` – name of the new branch  
- **Effects:** No change to working graph; simply adds a branch label.  


### `checkout(branchName)`

```cpp
void checkout(const std::string& branchName);
```

- **Description:** Switch `HEAD` to the tip of `branchName`. Rebuilds the working graph by replaying or fast-forwarding commits.  
- **Parameters:**  
  - `branchName` – must already exist  
- **Throws:** `runtime_error` if branch not found.  

### `listBranches()`
```cpp
std::vector<std::string> listBranches() const;
```

- **Description:** Return all branch names in the repository.  
- **Returns:** `vector<string>` of branch names.  


### `listCommits(branchName)`

```cpp
std::vector<Commit> listCommits(const std::string& branchName) const;
```

- **Description:** List `Commit` objects on `branchName`, from root → tip.  
- **Parameters:**  
  - `branchName` – branch to inspect  
- **Returns:** `vector<Commit>` in chronological order.  
- **Throws:** `runtime_error` if branch not found.  


### `getCommitGraph()`

```cpp
CommitGraph getCommitGraph() const;
```

- **Description:** Return a `CommitGraph` struct representing the DAG of all commits.  
- **Returns:** `CommitGraph` (maps `commitIds`, `parents`, `children`).  

### `merge(branchName, policy)`

```cpp
MergeResult merge(const std::string& branchName,
                  MergePolicy policy = MergePolicy::OURS);
```

- **Description:** Merge `branchName` into the current branch (`HEAD`).  
- **Parameters:**  
  - `branchName` – branch to merge from  
  - `policy`     – conflict resolution strategy  
- **Returns:** `MergeResult` containing new merge commit ID and any conflicts.  
- **Effects:**  
  - Fast-forward if possible, else create a two-parent commit.  
  - Rebuild working graph to merged state.  


### `graph()`

```cpp
const Graph& graph() const;
```

- **Description:** Access the current working‐tree `Graph` after checkout or staged mutators.  
- **Usage:**  
```cpp
  const auto& g = repo.graph();
  // use g.getNodes(), algorithms, snapshots, etc.
```

---

*End of Repository API reference.*  
