// src/Snapshot.cpp
#include "chronograph/Snapshot.h"
#include "chronograph/Event.h"
#include <algorithm>

namespace chronograph {

Snapshot::Snapshot(const Graph& graph, std::int64_t timestamp) {
  const auto& events = graph.getEventLog();
  const auto& checkpoints = graph.getCheckpoints();

  size_t startIdx = 0;
  // find latest checkpoint <= timestamp
  if (!checkpoints.empty()) {
    for (auto it = checkpoints.rbegin(); it != checkpoints.rend(); ++it) {
      if (it->timestamp <= timestamp) {
        nodes_ =it->nodes;
        edges_ = it->edges;
        outgoing_ = it->outgoing;
        incoming_ = it->incoming;
        startIdx = it->eventIndex;
        break;
      }
    }
  }

  // "replay" remaining events
  for (size_t i = startIdx; i < events.size(); ++i) {
    const auto& e = events[i];
    if (e.timestamp > timestamp) break;

    switch (e.type) {
      case EventType::ADD_NODE:
        nodes_[e.entityId] = Node{e.entityId, e.payload};
        outgoing_.emplace(e.entityId, std::vector<std::string>{});
        incoming_.emplace(e.entityId, std::vector<std::string>{});
        break;

      case EventType::DEL_NODE: {
        // remove all outgoing edges
        if (auto oit = outgoing_.find(e.entityId); oit != outgoing_.end()) {
          for (auto& eid : oit->second) {
            // remove from edges_
            if (auto eit = edges_.find(eid); eit != edges_.end()) {
              // also clean up incoming_ on that edge’s target
              incoming_[eit->second.to].erase(
                std::remove(incoming_[eit->second.to].begin(),
                            incoming_[eit->second.to].end(),
                            eid),
                incoming_[eit->second.to].end()
              );
              edges_.erase(eit);
            }
          }
          outgoing_.erase(oit);
        }
        // remove all incoming edges
        if (auto iit = incoming_.find(e.entityId); iit != incoming_.end()) {
          for (auto& eid : iit->second) {
            if (auto eit = edges_.find(eid); eit != edges_.end()) {
              outgoing_[eit->second.from].erase(
                std::remove(outgoing_[eit->second.from].begin(),
                            outgoing_[eit->second.from].end(),
                            eid),
                outgoing_[eit->second.from].end()
              );
              edges_.erase(eit);
            }
          }
          incoming_.erase(iit);
        }
        nodes_.erase(e.entityId);
      } break;

      case EventType::UPDATE_NODE:
        if (auto nit = nodes_.find(e.entityId); nit != nodes_.end()) {
          for (auto& [k,v] : e.payload)
            nit->second.attributes[k] = v;
        }
        break;

      case EventType::ADD_EDGE:
        edges_[e.entityId] = Edge{e.entityId, e.from, e.to, e.payload};
        outgoing_[e.from].push_back(e.entityId);
        incoming_[e.to].push_back(e.entityId);
        break;

      case EventType::DEL_EDGE:
        // use recorded endpoints
        outgoing_[e.from].erase(
          std::remove(outgoing_[e.from].begin(),
                      outgoing_[e.from].end(),
                      e.entityId),
          outgoing_[e.from].end()
        );
        incoming_[e.to].erase(
          std::remove(incoming_[e.to].begin(),
                      incoming_[e.to].end(),
                      e.entityId),
          incoming_[e.to].end()
        );
        edges_.erase(e.entityId);
        break;

      case EventType::UPDATE_EDGE:
        if (auto eit = edges_.find(e.entityId); eit != edges_.end()) {
          for (auto& [k,v] : e.payload)
            eit->second.attributes[k] = v;
        }
        break;
    }
  }
}

}  // namespace chronograph
