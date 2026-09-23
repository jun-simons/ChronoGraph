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

## 2. Merge & Conflict Types

`MergePolicy`, `Resolution`, `Conflict`, `EntityVersion` and `MergeResult` are covered in [Merging & Conflicts](merge.md).

## 3. The `Repository` Class

Provides a versioned overlay on top of `Graph`.

```cpp
class Repository {
public:
    static Repository init(const std::string& rootBranch = "main");
    // staging mutators…
    // commit, branch, checkout, listBranches, listCommits, getCommitGraph, merge
    const Graph& graph() const;
    // history: currentBranch, headCommit, getCommit, graphAt, diff
    // persistence: exportData, fromData
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
- **Throws:** `runtime_error` if branch not found, or if there are uncommitted changes and the branch points at a different commit (switching to a branch at the same commit carries uncommitted work along, like `git checkout`).  

### `hasUncommittedChanges()`

```cpp
bool hasUncommittedChanges() const;
```

- **Description:** `true` if the working graph has events that haven't been committed yet.  

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

- **Description:** Merge `branchName` into the current branch (`HEAD`): a no-op, a fast-forward, or a three-way merge against the lowest common ancestor with per-attribute conflict detection.  
- **Parameters:**  
  - `branchName` – branch to merge from  
  - `policy`     – how conflicts are settled (`OURS`, `THEIRS`, `ATTRIBUTE_UNION`, `INTERACTIVE`)  
- **Returns:** `MergeResult` with the merge commit ID (empty while an interactive merge is pending) and every conflict found.  
- **Throws:** `runtime_error` if the branch is not found, there are uncommitted changes, or a merge is already in progress.  
- **See:** [Merging & Conflicts](merge.md) for the rules, policies and the interactive API (`isMerging`, `mergeConflicts`, `resolveConflict`, `abortMerge`).  


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

## Inspecting History

A **ref** is a branch name or a commit ID; branch names take precedence. Unknown refs throw `runtime_error`.

```cpp
const std::string& currentBranch() const;   // checked-out branch
const std::string& headCommit() const;      // commit HEAD points at
const Commit& getCommit(const std::string& commitId) const;

Graph      graphAt(const std::string& ref) const;
DiffResult diff(const std::string& fromRef, const std::string& toRef) const;
```

- `graphAt(ref)` rebuilds the **committed** graph at any branch or commit without touching `HEAD` or the working tree. The returned `Graph` carries that commit's full event log, so [snapshots](snapshot.md) and [temporal queries](temporal.md) work on it.
- `diff(from, to)` compares the committed graphs at two refs, e.g. `repo.diff("main", "dev")`.

```cpp
auto changes = repo.diff("main", "feature");
Snapshot lastWeek(repo.graphAt("main"), ts - 7 * 24 * 3600);
```

## Persistence

See [Saving & Loading](io.md). `exportData()` / `fromData()` convert a repository to and from a plain `RepositoryData` struct, which is what the io layer (or your own storage) reads and writes.

---

*End of Repository API reference.*  
