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