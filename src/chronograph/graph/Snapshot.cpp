// src/Snapshot.cpp
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/Event.h>
#include <chronograph/graph/Graph.h>

namespace chronograph {

Snapshot::Snapshot(const Graph& graph, std::int64_t timestamp) {
  const auto& events = graph.getEventLog();
  const auto& checkpoints = graph.getCheckpoints();

  // Start from the latest checkpoint whose events all happened at or before
  // `timestamp` (a checkpoint's timestamp is the max over its events)
  size_t startIdx = 0;
  for (auto it = checkpoints.rbegin(); it != checkpoints.rend(); ++it) {
    if (it->timestamp <= timestamp) {
      state_ = it->state;
      startIdx = it->eventIndex;
      break;
    }
  }

  // Replay the remaining events up to `timestamp`. The log is in causal order
  // but not necessarily sorted by timestamp (merges append older events), so
  // filter rather than stopping at the first later event.
  for (size_t i = startIdx; i < events.size(); ++i) {
    if (events[i].timestamp <= timestamp) {
      state_.apply(events[i]);
    }
  }
}

}  // namespace chronograph
