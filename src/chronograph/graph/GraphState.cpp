// src/GraphState.cpp
#include <chronograph/graph/GraphState.h>
#include <algorithm>

namespace chronograph {

namespace {
    using Adjacency = std::unordered_map<std::string, std::vector<std::string>>;

    // Remove `edgeId` from adj[key] without creating an entry for `key`
    void eraseFrom(Adjacency& adj, const std::string& key, const std::string& edgeId) {
        auto it = adj.find(key);
        if (it == adj.end()) return;
        auto& ids = it->second;
        ids.erase(std::remove(ids.begin(), ids.end(), edgeId), ids.end());
    }
} // anonymous

void GraphState::removeEdge(const std::string& id) {
    auto it = edges.find(id);
    if (it == edges.end()) return;
    eraseFrom(outgoing, it->second.from, id);
    eraseFrom(incoming, it->second.to, id);
    edges.erase(it);
}

void GraphState::apply(const Event& e) {
    switch (e.type) {
      case EventType::ADD_NODE:
        nodes[e.entityId] = Node{e.entityId, e.payload};
        outgoing.try_emplace(e.entityId);
        incoming.try_emplace(e.entityId);
        break;

      case EventType::DEL_NODE: {
        // Cascade to incident edges (copy the lists: removeEdge mutates them)
        std::vector<std::string> incident;
        if (auto it = outgoing.find(e.entityId); it != outgoing.end())
            incident.insert(incident.end(), it->second.begin(), it->second.end());
        if (auto it = incoming.find(e.entityId); it != incoming.end())
            incident.insert(incident.end(), it->second.begin(), it->second.end());
        for (const auto& eid : incident) removeEdge(eid);

        nodes.erase(e.entityId);
        outgoing.erase(e.entityId);
        incoming.erase(e.entityId);
      } break;

      case EventType::UPDATE_NODE:
        if (auto it = nodes.find(e.entityId); it != nodes.end()) {
            for (const auto& [k, v] : e.payload) it->second.attributes[k] = v;
        }
        break;

      case EventType::ADD_EDGE:
        // Re-adding an existing ID replaces it rather than duplicating adjacency
        removeEdge(e.entityId);
        edges[e.entityId] = Edge{e.entityId, e.from, e.to, e.payload, e.timestamp};
        outgoing[e.from].push_back(e.entityId);
        incoming[e.to].push_back(e.entityId);
        break;

      case EventType::DEL_EDGE:
        removeEdge(e.entityId);
        break;

      case EventType::UPDATE_EDGE:
        if (auto it = edges.find(e.entityId); it != edges.end()) {
            for (const auto& [k, v] : e.payload) it->second.attributes[k] = v;
        }
        break;
    }
}

void GraphState::clear() {
    nodes.clear();
    edges.clear();
    outgoing.clear();
    incoming.clear();
}

}  // namespace chronograph
