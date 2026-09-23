// src/chronograph/repo/MergePlanner.h  (private to the repo library)
#pragma once

#include <chronograph/graph/Graph.h>
#include <chronograph/repo/Merge.h>
#include <cstdint>
#include <vector>

namespace chronograph {
namespace detail {

struct MergePlan {
    Graph merged;                     // ours plus the events that merge theirs in
    std::vector<Conflict> conflicts;  // every conflict found, settled per policy
                                      // (INTERACTIVE settles them as OURS for now)
    std::int64_t timestamp = 0;       // stamp for synthesized events
};

/**
 * Three-way merge of `theirs` into `ours`, relative to their common ancestor
 * `base`. Pure: the inputs are not modified.
 *
 * Every node and edge is compared across the three versions:
 *   • changed on one side only  -> that side's version
 *   • changed on both sides     -> attributes merged key by key; a key (or an
 *                                  edge's endpoints) set differently on each
 *                                  side, or a delete against a change, is a
 *                                  conflict settled by `policy`
 *   • a node deleted on one side while the other still has edges to it is a
 *     conflict on that node; its edges are kept or dropped with it
 *
 * Entities taken wholesale from `theirs` are merged by replaying their
 * original events, so their history keeps its timestamps. Everything else is
 * written with ordinary mutators stamped with the latest timestamp in either
 * history (edges keep their creation time).
 */
MergePlan planMerge(const Graph& base, const Graph& ours, const Graph& theirs,
                    MergePolicy policy);

/// Settle one conflict in `g` (normally a pending merge's working graph).
/// Throws std::invalid_argument, leaving `g` unchanged, if the chosen version
/// is an edge whose endpoint no longer exists.
void applyResolution(Graph& g, const Conflict& conflict, Resolution resolution,
                     std::int64_t timestamp);

}  // namespace detail
}  // namespace chronograph
