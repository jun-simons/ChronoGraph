# Merging & Conflicts

**Headers:** `include/chronograph/repo/Merge.h` (types), `Repository.h` (`merge` and the interactive API)

```cpp
MergeResult merge(const std::string& branchName, MergePolicy policy = MergePolicy::OURS);
```

## How a merge works

1. **Already merged:** if `branchName` is already contained in `HEAD`, nothing happens.
2. **Fast-forward:** if `HEAD` is an ancestor of the branch, `HEAD` moves to the branch tip.
3. **Three-way merge:** otherwise both sides are compared against their **lowest common ancestor**, the most recent commit both histories share (earlier merges between the branches count). Each node and edge is compared across base, ours and theirs:

| Base → ours | Base → theirs | Result |
|---|---|---|
| unchanged | changed | theirs |
| changed | unchanged | ours |
| same change on both sides | | that change |
| changed | changed differently | attributes merged key by key; a **conflict** only where they clash |

Changes on different attributes of the same node combine without a conflict. A conflict is recorded when:

- **`UPDATE_UPDATE`**: both sides set the same attribute key (or an edge's endpoints) to different values.
- **`ADD_ADD`**: both sides added the same ID with clashing attributes or endpoints. Identical additions don't conflict.
- **`DEL_UPDATE`**: one side deleted an entity that the other side changed. It also covers a node deleted on one side while the other side still has edges to it (listed in `dependentEdges`). Those edges are kept or dropped along with the node, so a merge never leaves an edge pointing at a missing node.

## Policies

Policies only decide conflicts; changes made on just one side are always merged.

| Policy | Attribute / endpoint clashes | Delete vs. change |
|---|---|---|
| `OURS` | our value | our side (deleted stays deleted) |
| `THEIRS` | their value | their side |
| `ATTRIBUTE_UNION` | our value | the entity survives, with the changed side's version |
| `INTERACTIVE` | left pending | left pending |

With the automatic policies, the merge commit (two parents: `HEAD` and the branch tip) is created immediately. `MergeResult::conflicts` lists every conflict that was settled, so you can review what the policy decided.

## Conflicts

```cpp
struct EntityVersion {
    std::map<std::string, std::string> attributes;
    std::string from, to;              // edges only
    std::int64_t createdTimestamp = 0; // edges only
};

struct Conflict {
    enum Kind { ADD_ADD, DEL_UPDATE, UPDATE_UPDATE } kind;
    enum EntityKind { NODE, EDGE } entity;
    std::string id;
    std::optional<EntityVersion> base, ours, theirs;  // nullopt = absent / deleted
    std::vector<std::string> keys;                    // clashing attribute keys
    bool endpoints = false;                           // edge endpoints clash
    std::vector<Edge> dependentEdges;                 // node conflicts: edges that follow the node
};
```

## Interactive merges

With `MergePolicy::INTERACTIVE` and at least one conflict, the merge stops before committing:

- `result.mergeCommitId` is empty and `isMerging()` is true.
- The working graph holds the merged state. All non-conflicting changes are applied, and conflicts are provisionally settled as `OURS`.
- `checkout`, another `merge`, and `exportData` / saving throw until the merge is committed or aborted. Mutators still work, so you can edit the working graph by hand.

```cpp
bool isMerging() const;
const std::vector<Conflict>& mergeConflicts() const;   // still unresolved
void resolveConflict(const Conflict& c, Resolution r); // OURS, THEIRS or MANUAL
void abortMerge();                                     // back to HEAD
std::string commit(const std::string& message = "");   // creates the merge commit
```

- `Resolution::OURS` / `THEIRS` apply that side's version, still keeping non-clashing changes from both sides.
- `Resolution::MANUAL` accepts whatever the working graph holds after your own edits.
- `commit()` throws while conflicts remain. Once they're resolved it creates the two-parent merge commit (default message `Merge branch '<name>' into <branch>`).

```cpp
auto result = repo.merge("dev", MergePolicy::INTERACTIVE);
for (const auto& c : repo.mergeConflicts()) {     // copy: resolving shrinks the list
    if (c.entity == Conflict::NODE && c.keys == std::vector<std::string>{"status"}) {
        repo.updateNode(c.id, {{"status", "reviewed"}}, now);
        repo.resolveConflict(c, Resolution::MANUAL);
    } else {
        repo.resolveConflict(c, Resolution::THEIRS);
    }
}
repo.commit();
```

## Timestamps and history

- **Taken wholesale from their side:** entities are merged by replaying their original events, so a `Snapshot` of the merged graph shows those changes at the time they were made.
- **Combined or conflict-settled:** these changes are written as new events stamped with the latest timestamp in either history. Edges keep their original creation time.
- **Attributes can't be removed by an update:** when a merge drops an attribute, the node or edge is deleted and re-created (a node keeps its edges).

## Limitations

- **Criss-cross histories:** when several equally good common ancestors exist, one is picked deterministically. There's no recursive merge of the candidate bases.
- **Cascaded edge deletions win:** deleting a node on one side also deletes its edges. If the other side left those edges untouched, the deletions stand even when the node itself is kept by a resolution.
