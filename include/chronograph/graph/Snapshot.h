// include/chronograph/Snapshot.h
#pragma once

#include <cstdint>

#include <chronograph/graph/GraphView.h>

namespace chronograph {

// forward declaration
class Graph;

/// The graph's state at a point in time; read it through GraphView's accessors
class Snapshot : public GraphView {
public:
    // Build snapshot by replaying every event with timestamp <= `timestamp`
    Snapshot(const Graph& graph, std::int64_t timestamp);
};

}  // namespace chronograph
