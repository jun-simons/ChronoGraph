// include/chronograph/Graph.h
#pragma once

#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <chronograph/graph/GraphState.h>
#include <chronograph/graph/GraphView.h>
#include <chronograph/graph/Diff.h>
#include <chronograph/graph/Snapshot.h>
#include <cstdint>
#include <limits>
#include <vector>
#include <unordered_map>
#include <map>

namespace chronograph {

/// Contains the core functionality for Graphs in ChronoGraph
// * Handles nodes, edges, and events
// * Read accessors (getNodes, getEdges, ...) come from GraphView
class Graph : public GraphView {
public:
    Graph() = default;

    /// Append a fully-formed event to the log and apply it to the live state.
    // * Low-level: no validation. Used to replay history (e.g. on checkout/merge).
    void addEvent(const Event& event);

    // Mutators: each validates its input, then records an Event and applies it.
    // * Throw std::invalid_argument on duplicate IDs, missing nodes/edges, or
    //   edges whose endpoints don't exist; the graph is unchanged on throw.
    // * delNode also records a DEL_EDGE for every incident edge (before the DEL_NODE).
    void addNode(const std::string& id,
                 const std::map<std::string, std::string>& attrs,
                 std::int64_t timestamp);
    void delNode(const std::string& id, std::int64_t timestamp);
    void addEdge(const std::string& id,
                 const std::string& from,
                 const std::string& to,
                 const std::map<std::string, std::string>& attrs,
                 std::int64_t timestamp);
    void delEdge(const std::string& id, std::int64_t timestamp);
    void updateNode(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp);
    void updateEdge(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp);

    // Access event log
    const std::vector<Event>& getEventLog() const;
    // expose checkpoints so Snapshot can use them
    struct Checkpoint {
        std::int64_t timestamp;   // latest timestamp among events [0, eventIndex)
        size_t eventIndex;        // number of events folded into `state`
        GraphState state;
    };
    const std::vector<Checkpoint>& getCheckpoints() const;

    using DiffResult = chronograph::DiffResult;
    /// Changes between the graph's state at t1 and at t2
    DiffResult diff(std::int64_t t1, std::int64_t t2) const;

    // Clear state, event log and checkpoints
    void clearGraph();

private:
    // Append-only event history
    std::vector<Event> eventLog_;

    // Latest timestamp seen in eventLog_ (the log is not required to be sorted)
    std::int64_t maxTimestamp_ = std::numeric_limits<std::int64_t>::min();

    // Checkpoint storage & parameters
    std::vector<Checkpoint> checkpoints_;
    static constexpr size_t kCheckpointInterval = 5000;
    void maybeCreateCheckpoint();
    void record(Event e);  // assign an ID, then addEvent()
};

}  // namespace chronograph