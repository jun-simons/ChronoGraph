// include/versioned_graph/Graph.h
#pragma once

#include "Event.h"
#include "Node.h"
#include "Edge.h"
#include <vector>
#include <unordered_map>

namespace versioned_graph {

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

private:
    std::vector<Event> eventLog_;
};

}  // namespace versioned_graph