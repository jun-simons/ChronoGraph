// src/chronograph/repo/MergePlanner.cpp
#include "MergePlanner.h"

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <utility>

namespace chronograph {
namespace detail {

namespace {

using Attrs    = std::map<std::string, std::string>;
using Version  = std::optional<EntityVersion>;
using Entity   = Conflict::EntityKind;
using Endpoints = std::pair<std::string, std::string>;

// ——— Versions ———

// Content equality; an edge's creation time is bookkeeping, not content
bool sameContent(const Version& a, const Version& b) {
    if (!a || !b) return !a && !b;
    return a->attributes == b->attributes && a->from == b->from && a->to == b->to;
}

Version versionOf(const GraphView& g, Entity kind, const std::string& id) {
    if (kind == Conflict::NODE) {
        auto it = g.getNodes().find(id);
        if (it == g.getNodes().end()) return std::nullopt;
        return EntityVersion{it->second.attributes, {}, {}, 0};
    }
    auto it = g.getEdges().find(id);
    if (it == g.getEdges().end()) return std::nullopt;
    const Edge& e = it->second;
    return EntityVersion{e.attributes, e.from, e.to, e.createdTimestamp};
}

Endpoints endpointsOf(const Version& v) {
    return v ? Endpoints{v->from, v->to} : Endpoints{};
}

// ——— Three-way rules ———

// The value after merging base -> ours and base -> theirs, or nullopt if both
// sides changed it differently
template <typename T>
std::optional<T> threeWay(const T& base, const T& ours, const T& theirs) {
    if (ours == theirs) return ours;
    if (ours == base)   return theirs;
    if (theirs == base) return ours;
    return std::nullopt;
}

struct AttributeMerge {
    Attrs merged;
    std::vector<std::string> clashes;
};

// Key-by-key three-way merge; clashing keys take `winner`'s value
AttributeMerge mergeAttributes(const Attrs& base, const Attrs& ours,
                               const Attrs& theirs, Resolution winner) {
    std::set<std::string> keys;
    for (const Attrs* m : {&base, &ours, &theirs}) {
        for (const auto& [k, _] : *m) keys.insert(k);
    }
    auto lookup = [](const Attrs& m, const std::string& k) -> std::optional<std::string> {
        auto it = m.find(k);
        return it == m.end() ? std::nullopt : std::optional<std::string>(it->second);
    };

    AttributeMerge result;
    for (const auto& k : keys) {
        auto b = lookup(base, k), o = lookup(ours, k), t = lookup(theirs, k);
        std::optional<std::string> value;
        if (auto merged = threeWay(b, o, t)) {
            value = *merged;
        } else {
            result.clashes.push_back(k);
            value = winner == Resolution::THEIRS ? t : o;
        }
        if (value) result.merged[k] = *value;
    }
    return result;
}

// The version a conflict settles to when `winner` takes the clashes
Version resolvedVersion(const Conflict& c, Resolution winner) {
    const Version& preferred = winner == Resolution::THEIRS ? c.theirs : c.ours;
    if (!c.ours || !c.theirs) return preferred;  // delete vs. change: all or nothing

    const EntityVersion none;
    const EntityVersion& b = c.base ? *c.base : none;
    EntityVersion v = *preferred;
    v.attributes = mergeAttributes(b.attributes, c.ours->attributes,
                                   c.theirs->attributes, winner).merged;
    if (auto ends = threeWay(endpointsOf(c.base), endpointsOf(c.ours), endpointsOf(c.theirs))) {
        std::tie(v.from, v.to) = *ends;
    }
    return v;
}

Resolution winnerFor(MergePolicy policy, const Conflict& c) {
    switch (policy) {
      case MergePolicy::THEIRS:          return Resolution::THEIRS;
      case MergePolicy::ATTRIBUTE_UNION: return c.ours ? Resolution::OURS : Resolution::THEIRS;
      case MergePolicy::OURS:
      case MergePolicy::INTERACTIVE:     return Resolution::OURS;
    }
    return Resolution::OURS;
}

// ——— Writing a version into a graph ———

bool dropsKeys(const Attrs& from, const Attrs& to) {
    return std::any_of(from.begin(), from.end(),
                       [&](const auto& kv) { return !to.count(kv.first); });
}

Attrs changedKeys(const Attrs& from, const Attrs& to) {
    Attrs changed;
    for (const auto& [k, v] : to) {
        auto it = from.find(k);
        if (it == from.end() || it->second != v) changed[k] = v;
    }
    return changed;
}

std::vector<Edge> incidentEdges(const Graph& g, const std::string& node) {
    std::vector<Edge> out;
    std::unordered_set<std::string> seen;
    for (const auto* adj : {&g.getOutgoing(), &g.getIncoming()}) {
        auto it = adj->find(node);
        if (it == adj->end()) continue;
        for (const auto& eid : it->second) {
            if (seen.insert(eid).second) out.push_back(g.getEdges().at(eid));
        }
    }
    return out;
}

// Make entity `id` in `g` match `target` using validated mutators.
// Attributes can't be removed by an update, so dropping one re-creates the
// entity (a node keeps its edges). Edges are (re)added at their creation time.
void setEntity(Graph& g, Entity kind, const std::string& id,
               const Version& target, std::int64_t ts) {
    const Version current = versionOf(g, kind, id);
    if (sameContent(current, target)) return;

    if (kind == Conflict::NODE) {
        if (!target) { g.delNode(id, ts); return; }
        if (!current) { g.addNode(id, target->attributes, ts); return; }
        if (dropsKeys(current->attributes, target->attributes)) {
            const auto edges = incidentEdges(g, id);
            g.delNode(id, ts);
            g.addNode(id, target->attributes, ts);
            for (const auto& e : edges) {
                g.addEdge(e.id, e.from, e.to, e.attributes, e.createdTimestamp);
            }
        } else {
            g.updateNode(id, changedKeys(current->attributes, target->attributes), ts);
        }
        return;
    }

    // Validate before touching anything, so a failure leaves `g` unchanged
    if (target) {
        for (const auto& n : {target->from, target->to}) {
            if (!g.hasNode(n)) {
                throw std::invalid_argument("Cannot restore edge '" + id + "': node '" +
                                            n + "' does not exist");
            }
        }
    }
    const bool recreate = current && (!target || current->from != target->from ||
                                      current->to != target->to ||
                                      dropsKeys(current->attributes, target->attributes));
    if (recreate) g.delEdge(id, ts);
    if (!target) return;
    if (!current || recreate) {
        g.addEdge(id, target->from, target->to, target->attributes, target->createdTimestamp);
    } else {
        g.updateEdge(id, changedKeys(current->attributes, target->attributes), ts);
    }
}

// ——— Planning ———

// Where one entity should end up, and whether to get there by replaying theirs
struct Decision {
    Version target;
    bool takeTheirs = false;
};
using Decisions = std::map<std::string, Decision>;  // ordered: deterministic output

std::set<std::string> idsOf(Entity kind, std::initializer_list<const Graph*> graphs) {
    std::set<std::string> ids;
    for (const Graph* g : graphs) {
        if (kind == Conflict::NODE) for (const auto& [id, _] : g->getNodes()) ids.insert(id);
        else                        for (const auto& [id, _] : g->getEdges()) ids.insert(id);
    }
    return ids;
}

std::int64_t latestTimestamp(const Graph& a, const Graph& b) {
    std::int64_t latest = std::numeric_limits<std::int64_t>::min();
    for (const Graph* g : {&a, &b}) {
        for (const auto& e : g->getEventLog()) latest = std::max(latest, e.timestamp);
    }
    return latest == std::numeric_limits<std::int64_t>::min() ? 0 : latest;
}

bool isNodeEvent(EventType t) {
    return t == EventType::ADD_NODE || t == EventType::UPDATE_NODE || t == EventType::DEL_NODE;
}

// Their events since the base, grouped by entity, in log order
std::map<std::pair<Entity, std::string>, std::vector<Event>>
eventsSinceBase(const Graph& base, const Graph& theirs) {
    std::unordered_set<std::string> seen;
    for (const auto& e : base.getEventLog()) seen.insert(e.id);

    std::map<std::pair<Entity, std::string>, std::vector<Event>> byEntity;
    for (const auto& e : theirs.getEventLog()) {
        if (seen.count(e.id)) continue;
        const Entity kind = isNodeEvent(e.type) ? Conflict::NODE : Conflict::EDGE;
        byEntity[{kind, e.entityId}].push_back(e);
    }
    return byEntity;
}

}  // namespace

MergePlan planMerge(const Graph& base, const Graph& ours, const Graph& theirs,
                    MergePolicy policy) {
    MergePlan plan{ours, {}, latestTimestamp(ours, theirs)};

    // 1) Decide every node and edge
    auto classify = [&](Entity kind) {
        Decisions out;
        for (const auto& id : idsOf(kind, {&base, &ours, &theirs})) {
            const Version b = versionOf(base, kind, id);
            const Version o = versionOf(ours, kind, id);
            const Version t = versionOf(theirs, kind, id);
            Decision& d = out[id];

            if (sameContent(o, t) || sameContent(t, b)) { d.target = o; continue; }
            if (sameContent(o, b)) { d.target = t; d.takeTheirs = true; continue; }

            // Both sides changed it
            Conflict c{!b ? Conflict::ADD_ADD
                          : (!o || !t) ? Conflict::DEL_UPDATE : Conflict::UPDATE_UPDATE,
                       kind, id, b, o, t, {}, false, {}};
            if (o && t) {
                c.keys = mergeAttributes(b ? b->attributes : Attrs{}, o->attributes,
                                         t->attributes, Resolution::OURS).clashes;
                c.endpoints = kind == Conflict::EDGE &&
                    !threeWay(endpointsOf(b), endpointsOf(o), endpointsOf(t));
                if (c.keys.empty() && !c.endpoints) {  // compatible changes
                    d.target = resolvedVersion(c, Resolution::OURS);
                    continue;
                }
            }
            d.target = resolvedVersion(c, winnerFor(policy, c));
            plan.conflicts.push_back(std::move(c));
        }
        return out;
    };
    Decisions nodes = classify(Conflict::NODE);
    Decisions edges = classify(Conflict::EDGE);

    // 2) Edges left pointing at a node that one side deleted: the node
    //    becomes (or already is) a conflict, and its edges follow it
    std::map<std::string, std::vector<Edge>> dependents;
    for (const auto& [id, d] : edges) {
        if (!d.target) continue;
        const Edge e{id, d.target->from, d.target->to, d.target->attributes,
                     d.target->createdTimestamp};
        for (const auto& n : std::set<std::string>{e.from, e.to}) {
            auto it = nodes.find(n);
            if (it == nodes.end() || !it->second.target) dependents[n].push_back(e);
        }
    }
    for (const auto& entry : dependents) {
        const std::string& nodeId = entry.first;  // (lambdas can't capture bindings in C++17)
        const std::vector<Edge>& deps = entry.second;
        auto existing = std::find_if(plan.conflicts.begin(), plan.conflicts.end(),
            [&](const Conflict& c) { return c.entity == Conflict::NODE && c.id == nodeId; });
        if (existing == plan.conflicts.end()) {
            plan.conflicts.push_back(Conflict{
                Conflict::DEL_UPDATE, Conflict::NODE, nodeId,
                versionOf(base, Conflict::NODE, nodeId),
                versionOf(ours, Conflict::NODE, nodeId),
                versionOf(theirs, Conflict::NODE, nodeId), {}, false, {}});
            existing = std::prev(plan.conflicts.end());
        }
        existing->dependentEdges = deps;

        Decision& node = nodes[nodeId];
        node = Decision{resolvedVersion(*existing, winnerFor(policy, *existing)), false};
        if (!node.target) {
            for (const auto& e : deps) edges[e.id] = Decision{};
        }
    }

    // 3) Write the decisions into a copy of ours. Order matters: edges that
    //    disappear go first (so node deletions never cascade into them), then
    //    nodes, then the surviving edges, which also restores any edge a node
    //    change cascaded away.
    Graph& g = plan.merged;
    const auto theirEvents = eventsSinceBase(base, theirs);
    auto reach = [&](Entity kind, const std::string& id, const Decision& d) {
        if (d.takeTheirs) {
            if (auto it = theirEvents.find({kind, id}); it != theirEvents.end()) {
                for (const auto& e : it->second) g.addEvent(e);
            }
        }
        setEntity(g, kind, id, d.target, plan.timestamp);  // no-op once it matches
    };
    for (const auto& [id, d] : edges) if (!d.target && g.hasEdge(id)) reach(Conflict::EDGE, id, d);
    for (const auto& [id, d] : nodes) reach(Conflict::NODE, id, d);
    for (const auto& [id, d] : edges) if (d.target) reach(Conflict::EDGE, id, d);

    return plan;
}

void applyResolution(Graph& g, const Conflict& conflict, Resolution resolution,
                     std::int64_t timestamp) {
    if (resolution == Resolution::MANUAL) return;

    const Version target = resolvedVersion(conflict, resolution);
    setEntity(g, conflict.entity, conflict.id, target, timestamp);

    // A node that survives takes the edges that depend on it along
    if (conflict.entity == Conflict::NODE && target) {
        for (const auto& e : conflict.dependentEdges) {
            if (!g.hasEdge(e.id) && g.hasNode(e.from) && g.hasNode(e.to)) {
                g.addEdge(e.id, e.from, e.to, e.attributes, e.createdTimestamp);
            }
        }
    }
}

}  // namespace detail
}  // namespace chronograph
