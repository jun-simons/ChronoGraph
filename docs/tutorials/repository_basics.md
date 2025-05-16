# Repository Basics

ChronoGraph provides a Git-style repository layer on top of your graph. You can stage changes, commit snapshots, create branches, switch between graph branches, and merge them together.

## 1. Initializing a Repository

Start with a fresh repo and a default branch (`main`):

```cpp
#include <chronograph/repo/Repository.h>

auto repo = chronograph::Repository::init("main");
```

Under the hood this:
- Creates an initial “root” commit (empty delta) on main
- Sets HEAD → main
- Prepares an in-memory working graph for your edits


---

## 2. Staging Changes

All the familiar graph mutators are available on the repo’s **working graph**. They append events to the repo’s log but don’t become part of any commit until you call `commit()`.

```cpp
// Stage node & edge additions at timestamps 1,2…
repo.addNode("A", {{"label","Alice"}}, /*ts=*/1);
repo.addNode("B", {{"label","Bob"}},   /*ts=*/2);
repo.addEdge("e1","A","B", {{"weight","1"}}, /*ts=*/3);

// These changes are in the repo.graph().getNodes()/getEdges(),
// but not yet in any commit.
```

---

## 3. Committing Changes

To record all staged events as a new commit, call:

```cpp
auto commitId = repo.commit("Added A, B nodes and edge e1");
// commitId is a new UUID-like string
```

- All events since the last commit become the delta of this new `Commit`
- The current branch pointer (`main`) moves to this new commit
- `HEAD` stays on the same branch

---

## 4. Branching & Checkout

Create lightweight branches and switch HEAD between them:

```cpp
// Create a new branch 'dev' at the current commit
repo.branch("dev");

// Move HEAD → dev (replay or fast-forward the working graph)
repo.checkout("dev");
```
- `branch(name)` doesn’t change your working graph—just records a pointer
- `checkout(name)` rebuilds the working graph to match that branch’s tip

---

## 5. Listing Branches & Commits

Inspect your repo history:

```cpp
// List all branch names
auto branches = repo.listBranches();  // e.g. {"main","dev"}

// List commits on a branch (root → tip)
auto commits = repo.listCommits("main");
for (auto& c : commits) {
    std::cout << c.id << " : " << c.message << "\n";
}
```

---

## 6. Viewing the Commit DAG

Get a compact DAG structure of all commits:

```cpp
auto dag = repo.getCommitGraph();
// dag.commitIds = all commit IDs
// dag.parents[cid] = vector of parent IDs
// dag.children[cid] = vector of child IDs
```

You can pretty-print this with printCommitGraph(...) (in printUtils.h), or export this to GraphViz.

---

## 7. Merging Branches

ChronoGraph supports a `merge()` API (with pluggable policies) to combine two lines of work:

```cpp
auto result = repo.merge("dev", MergePolicy::OURS);
if (!result.conflicts.empty()) {
  // resolve conflicts in INTERACTIVE mode
}
```

- Fast-forwards when possible
- Creates a two-parent commit on true three-way merges
- Returns any conflicts for manual resolution

---

## 8. Accessing the Working Graph

At any point, peek at the current branch’s live graph:

```cpp
const auto& g = repo.graph();
for (auto& [id,node] : g.getNodes()) {
    std::cout << id << "\n";
}
```

This is the same Graph API described in [Graph Basics](graph_basics.md)—snapshots, diff, algorithms, and DOT export all work on `repo.graph()` as well.

---

## Next

Continue to the [Algorithms Tutorial](algorithm_basics.md) to explore reachability, shortest paths, and connectivity analyses on your repository-backed graph.
