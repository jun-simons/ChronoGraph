// include/chronograph/Graph.h
#pragma once

#include <chronograph/graph/Event.h>
#include <chronograph/graph/Node.h>
#include <chronograph/graph/Edge.h>
#include <chronograph/graph/Snapshot.h>
#include <vector>
#include <unordered_map>
#include <map>

namespace chronograph {

/// Contains the core functionality for Graphs in ChronoGraph
// * Handles nodes, edges, and events
class Graph {
public:
    Graph() = default;

    // Append a raw event
    void addEvent(const Event& event);

    // Mutators
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
        std::int64_t timestamp;
        size_t eventIndex;
        std::unordered_map<std::string, Node> nodes;
        std::unordered_map<std::string, Edge> edges;
        std::unordered_map<std::string, std::vector<std::string>> outgoing;
        std::unordered_map<std::string, std::vector<std::string>> incoming;
    };
    const std::vector<Checkpoint>& getCheckpoints() const;

    struct DiffResult {
        // Nodes
        std::vector<Node> nodesAdded;
        std::vector<std::string> nodesRemoved;
        std::vector<std::pair<Node,Node>> nodesUpdated;   // {before, after}
  
        // Edges
        std::vector<Edge> edgesAdded;
        std::vector<std::string> edgesRemoved;
        std::vector<std::pair<Edge,Edge>> edgesUpdated;   // {before, after}
      };
    DiffResult diff(std::int64_t t1, std::int64_t t2) const;

    // Access current state
    const std::unordered_map<std::string, Node>& getNodes() const;
    const std::unordered_map<std::string, Edge>& getEdges() const;
    const std::unordered_map<std::string, std::vector<std::string>>& getOutgoing() const;
    const std::unordered_map<std::string, std::vector<std::string>>& getIncoming() const;

    // Apply a recorded Event to this graph’s state (no logging, no checkpoints)
    void applyEvent(const Event& event);
    // Clear all in-memory state (nodes, edges, adjacency) but keep eventLog_ intact
    void clearStateKeepLog();
    // Clear both in memory graph and branch-local events
    void clearGraph();

private:
    // Append-only event history
    std::vector<Event> eventLog_;

    // Graph state containers
    std::unordered_map<std::string, Node> nodes_;
    std::unordered_map<std::string, Edge> edges_;
    // Adjacency maps for traversal
    std::unordered_map<std::string, std::vector<std::string>> outgoing_;
    std::unordered_map<std::string, std::vector<std::string>> incoming_;

    // Checkpoint storage & parameters
    std::vector<Checkpoint> checkpoints_;
    static constexpr size_t kCheckpointInterval = 5000;
    void maybeCreateCheckpoint(const Event& e);
};

}  // namespace chronograph