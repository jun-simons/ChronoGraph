#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/GraphView.h>
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/Diff.h>
#include <chronograph/graph/Temporal.h>
#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/algorithms/Connectivity.h>
#include <chronograph/repo/Repository.h>
#include <chronograph/io/Serialization.h>

#include <optional>

namespace py = pybind11;
using namespace chronograph;

namespace {

// --- Node, Edge & Event records ---
void bindRecords(py::module_& m) {
    py::class_<Node>(m, "Node")
        .def_readwrite("id", &Node::id)
        .def_readwrite("attributes", &Node::attributes)
        .def("__getitem__",
            [](const Node &n, const std::string &key) {
                auto it = n.attributes.find(key);
                if (it == n.attributes.end())
                    throw py::key_error("Key '" + key + "' not found");
                return it->second;
            })
        .def("__repr__",
            [](const Node &n){
                return "<Node id='" + n.id + "'>";
            })
        ;
    py::class_<Edge>(m, "Edge")
        .def_readwrite("id", &Edge::id)
        .def_readwrite("from",&Edge::from)
        .def_readwrite("to", &Edge::to)
        .def_readwrite("attributes", &Edge::attributes)
        .def_readwrite("created_timestamp", &Edge::createdTimestamp)
        .def("__repr__",
            [](const Edge &e){
                return "<Edge id='" + e.id + "' " + e.from + "->" + e.to + ">";
            })
        ;

    py::enum_<EventType>(m, "EventType")
        .value("ADD_NODE",    EventType::ADD_NODE)
        .value("DEL_NODE",    EventType::DEL_NODE)
        .value("ADD_EDGE",    EventType::ADD_EDGE)
        .value("DEL_EDGE",    EventType::DEL_EDGE)
        .value("UPDATE_NODE", EventType::UPDATE_NODE)
        .value("UPDATE_EDGE", EventType::UPDATE_EDGE);

    py::class_<Event>(m, "Event")
        .def_readonly("id",        &Event::id)
        .def_readonly("timestamp", &Event::timestamp)
        .def_readonly("type",      &Event::type)
        .def_readonly("entity_id", &Event::entityId)
        .def_readonly("payload",   &Event::payload)
        .def_readonly("from_",     &Event::from)
        .def_readonly("to",        &Event::to)
        .def("__repr__", [](const Event& e) {
            return "<Event " + py::repr(py::cast(e.type)).cast<std::string>() +
                   " '" + e.entityId + "' @" + std::to_string(e.timestamp) + ">";
        });
}

// --- GraphView, Graph, Snapshot & diffs ---
void bindGraph(py::module_& m) {
    // Shared read-only accessors; algorithms accept any GraphView.
    // Python never owns a bare GraphView (it has no constructor and a
    // protected destructor), so its holder must never delete.
    py::class_<GraphView, std::unique_ptr<GraphView, py::nodelete>>(m, "GraphView")
        .def("get_nodes", &GraphView::getNodes)
        .def("get_edges", &GraphView::getEdges)
        .def("get_outgoing", &GraphView::getOutgoing)
        .def("get_incoming", &GraphView::getIncoming)
        .def("has_node", &GraphView::hasNode, py::arg("id"))
        .def("has_edge", &GraphView::hasEdge, py::arg("id"))
        ;

    py::class_<DiffResult>(m, "DiffResult")
        .def_readonly("nodes_added",   &DiffResult::nodesAdded)
        .def_readonly("nodes_removed", &DiffResult::nodesRemoved)
        .def_readonly("nodes_updated", &DiffResult::nodesUpdated)
        .def_readonly("edges_added",   &DiffResult::edgesAdded)
        .def_readonly("edges_removed", &DiffResult::edgesRemoved)
        .def_readonly("edges_updated", &DiffResult::edgesUpdated)
        .def("empty", &DiffResult::empty)
        ;

    py::class_<Graph, GraphView>(m, "Graph")
        .def(py::init<>())
        .def("add_node", &Graph::addNode,
             py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("del_node", &Graph::delNode, py::arg("id"), py::arg("timestamp"))
        .def("update_node", &Graph::updateNode,
            py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("add_edge", &Graph::addEdge,
             py::arg("id"), py::arg("from"), py::arg("to"),
             py::arg("attrs"), py::arg("timestamp"))
        .def("del_edge", &Graph::delEdge, py::arg("id"), py::arg("timestamp"))
        .def("update_edge", &Graph::updateEdge,
            py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("get_event_log", &Graph::getEventLog)
        .def("diff", &Graph::diff, py::arg("t1"), py::arg("t2"))
        ;

    py::class_<Snapshot, GraphView>(m, "Snapshot")
        .def(py::init<const Graph&, std::int64_t>(), py::arg("graph"), py::arg("timestamp"))
        ;

    m.def("diff", &chronograph::diff, py::arg("before"), py::arg("after"),
          "Changes between two graph states (Graphs or Snapshots)");
}

// --- Algorithms (free functions) ---
void bindAlgorithms(py::module_& m) {
    auto alg = m.def_submodule("algorithms", "Graph algorithms (accept a Graph or Snapshot)");
    alg.def("is_reachable", &graph::algorithms::isReachable,
          py::arg("g"), py::arg("start"), py::arg("target"));
    alg.def("shortest_path", &graph::algorithms::shortestPath,
          py::arg("g"), py::arg("start"), py::arg("target"));
    alg.def("is_reachable_at", &graph::algorithms::isReachableAt,
          py::arg("g"), py::arg("start"), py::arg("target"), py::arg("timestamp"));
    alg.def("is_time_respecting_reachable", &graph::algorithms::isTimeRespectingReachable,
          py::arg("g"), py::arg("start"), py::arg("target"));
    alg.def("dijkstra", &graph::algorithms::dijkstra,
          py::arg("g"), py::arg("start"), py::arg("target"), py::arg("weight_key"));
    alg.def("weakly_connected_components", &graph::algorithms::weaklyConnectedComponents, py::arg("g"));
    alg.def("strongly_connected_components", &graph::algorithms::stronglyConnectedComponents, py::arg("g"));
    alg.def("has_cycle", &graph::algorithms::hasCycle, py::arg("g"));
    alg.def("topological_sort", &graph::algorithms::topologicalSort, py::arg("g"));
}

// --- Temporal queries ---
void bindTemporal(py::module_& m) {
    using temporal::TimeRange;
    auto tm = m.def_submodule("temporal", "Queries over a graph's event history");

    py::class_<TimeRange>(tm, "TimeRange")
        .def(py::init([](std::optional<std::int64_t> start, std::optional<std::int64_t> end) {
                 TimeRange r;
                 if (start) r.start = *start;
                 if (end)   r.end = *end;
                 return r;
             }),
             py::arg("start") = py::none(), py::arg("end") = py::none(),
             "Inclusive window [start, end]; an omitted bound is unbounded")
        .def_readwrite("start", &TimeRange::start)
        .def_readwrite("end", &TimeRange::end)
        .def("contains", &TimeRange::contains, py::arg("t"))
        ;

    tm.def("events_in_range", &temporal::eventsInRange, py::arg("g"), py::arg("range"));
    tm.def("node_history", &temporal::nodeHistory,
           py::arg("g"), py::arg("id"), py::arg("range") = TimeRange{});
    tm.def("edge_history", &temporal::edgeHistory,
           py::arg("g"), py::arg("id"), py::arg("range") = TimeRange{});
    tm.def("node_at", &temporal::nodeAt, py::arg("g"), py::arg("id"), py::arg("timestamp"));
    tm.def("edge_at", &temporal::edgeAt, py::arg("g"), py::arg("id"), py::arg("timestamp"));
    tm.def("change_times", &temporal::changeTimes, py::arg("g"), py::arg("range") = TimeRange{});
}

// --- Repository ---
void bindRepository(py::module_& m) {
    py::enum_<MergePolicy>(m, "MergePolicy")
        .value("OURS", MergePolicy::OURS)
        .value("THEIRS", MergePolicy::THEIRS)
        .value("ATTRIBUTE_UNION", MergePolicy::ATTRIBUTE_UNION)
        .value("INTERACTIVE", MergePolicy::INTERACTIVE)
        .export_values();

    py::class_<Conflict> conflict(m, "Conflict");
    py::enum_<Conflict::Kind>(conflict, "Kind")
        .value("ADD_ADD",       Conflict::ADD_ADD)
        .value("DEL_UPDATE",    Conflict::DEL_UPDATE)
        .value("UPDATE_UPDATE", Conflict::UPDATE_UPDATE);
    conflict
        .def_readonly("kind",    &Conflict::kind)
        .def_readonly("ours",    &Conflict::ours)
        .def_readonly("theirs",  &Conflict::theirs)
        ;

    py::class_<MergeResult>(m, "MergeResult")
        .def_readonly("merge_commit_id", &MergeResult::mergeCommitId)
        .def_readonly("conflicts",       &MergeResult::conflicts)
        ;

    py::class_<Commit>(m, "Commit")
        .def_readonly("id",      &Commit::id)
        .def_readonly("parents", &Commit::parents)
        .def_readonly("events",  &Commit::events)
        .def_readonly("message", &Commit::message)
        .def("__repr__", [](const Commit& c) {
            return "<Commit " + c.id + " '" + c.message + "'>";
        });

    py::class_<CommitGraph>(m, "CommitGraph")
        .def_readonly("commit_ids", &CommitGraph::commitIds)
        .def_readonly("parents",    &CommitGraph::parents)
        .def_readonly("children",   &CommitGraph::children)
        ;

    py::class_<Repository>(m, "Repository")
        .def_static("init", &Repository::init, py::arg("root_branch") = "main")
        .def("add_node", &Repository::addNode,
             py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("del_node", &Repository::delNode, py::arg("id"), py::arg("timestamp"))
        .def("update_node", &Repository::updateNode,
             py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("add_edge", &Repository::addEdge,
             py::arg("id"), py::arg("from"), py::arg("to"),
             py::arg("attrs"), py::arg("timestamp"))
        .def("del_edge", &Repository::delEdge, py::arg("id"), py::arg("timestamp"))
        .def("update_edge", &Repository::updateEdge,
             py::arg("id"), py::arg("attrs"), py::arg("timestamp"))
        .def("commit", &Repository::commit, py::arg("message") = "")
        .def("has_uncommitted_changes", &Repository::hasUncommittedChanges)
        .def("branch", &Repository::branch, py::arg("name"))
        .def("checkout", &Repository::checkout, py::arg("branch"))
        .def("list_branches", &Repository::listBranches)
        .def("list_commits", &Repository::listCommits, py::arg("branch"))
        .def("get_commit_graph", &Repository::getCommitGraph)
        .def("merge", &Repository::merge,
             py::arg("branch"), py::arg("policy") = MergePolicy::OURS)
        .def("graph", &Repository::graph, py::return_value_policy::reference_internal)
        .def("current_branch", &Repository::currentBranch)
        .def("head_commit", &Repository::headCommit)
        .def("get_commit", &Repository::getCommit, py::arg("commit_id"))
        .def("graph_at", &Repository::graphAt, py::arg("ref"))
        .def("diff", &Repository::diff, py::arg("from_ref"), py::arg("to_ref"))
        ;
}

// --- Saving & loading ---
void bindIo(py::module_& m) {
    // Bad file contents surface as FormatError (a ValueError)
    py::register_exception<io::FormatError>(m, "FormatError", PyExc_ValueError);

    m.def("save_graph", &io::saveGraph, py::arg("graph"), py::arg("path"));
    m.def("load_graph", &io::loadGraph, py::arg("path"));
    m.def("save_repository", &io::saveRepository, py::arg("repo"), py::arg("path"));
    m.def("load_repository", &io::loadRepository, py::arg("path"));
    m.def("graph_to_json", &io::graphToJson, py::arg("graph"));
    m.def("graph_from_json", &io::graphFromJson, py::arg("json"));
    m.def("repository_to_json", &io::repositoryToJson, py::arg("repo"));
    m.def("repository_from_json", &io::repositoryFromJson, py::arg("json"));
}

}  // namespace

PYBIND11_MODULE(chronograph, m) {
    m.doc() = "ChronoGraph C++ binding";

    bindRecords(m);
    bindGraph(m);
    bindAlgorithms(m);
    bindTemporal(m);
    bindRepository(m);
    bindIo(m);
}
