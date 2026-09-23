// include/chronograph/repo/Merge.h
#pragma once

#include <chronograph/graph/Edge.h>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chronograph {

/// How Repository::merge settles conflicts.
// * Changes made on only one side are always merged; policies only decide
//   what happens where both sides changed the same thing differently.
enum class MergePolicy {
    OURS,             // our side wins every conflict
    THEIRS,           // their side wins every conflict
    ATTRIBUTE_UNION,  // keep as much as possible: an entity deleted on one side
                      // but changed on the other survives; value clashes go to ours
    INTERACTIVE       // stop with conflicts pending for resolveConflict()
};

/// How to settle a single conflict during an interactive merge
enum class Resolution {
    OURS,    // our version (clashing attributes take our values)
    THEIRS,  // their version (clashing attributes take their values)
    MANUAL   // accept the working graph as it is, after your own edits
};

/// One side's version of a node or edge involved in a conflict
struct EntityVersion {
    std::map<std::string, std::string> attributes;
    std::string from, to;              // edges only
    std::int64_t createdTimestamp = 0; // edges only
};

/// A node or edge that both sides of a merge changed incompatibly
struct Conflict {
    enum Kind {
        ADD_ADD,        // both sides added it, differently
        DEL_UPDATE,     // one side deleted it; the other changed it (or, for a
                        // node, still has edges attached to it)
        UPDATE_UPDATE   // both sides changed it, differently
    };
    enum EntityKind { NODE, EDGE };

    Kind kind;
    EntityKind entity;
    std::string id;

    // Versions at the merge base and on each side (nullopt = absent/deleted)
    std::optional<EntityVersion> base, ours, theirs;

    // Attribute keys both sides set to different values
    std::vector<std::string> keys;
    // Edge endpoints changed differently on each side
    bool endpoints = false;
    // Node conflicts only: edges that reference this node on the side that kept
    // it. They are kept or dropped along with the node.
    std::vector<Edge> dependentEdges;
};

struct MergeResult {
    // The merge (or fast-forward) commit. Empty while an interactive merge
    // is waiting for conflicts to be resolved.
    std::string mergeCommitId;
    // Every conflict found. With OURS / THEIRS / ATTRIBUTE_UNION they have
    // already been settled by the policy; with INTERACTIVE they are pending.
    std::vector<Conflict> conflicts;
};

}  // namespace chronograph
