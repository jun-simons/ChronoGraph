#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>
#include <chronograph/graph/algorithms/Paths.h>
#include <chronograph/graph/algorithms/Connectivity.h>
#include <chronograph/repo/Repository.h>

namespace py = pybind11;
using namespace chronograph;

PYBIND11_MODULE(chronograph, m) {
    m.doc() = "ChronoGraph C++ binding";

    // --- Node & Edge objects ---
    py::class_<chronograph::Node>(m, "Node")
        .def_readwrite("id", &chronograph::Node::id)
        .def_readwrite("attributes", &chronograph::Node::attributes)
        .def("__getitem__",
            [](const chronograph::Node &n, const std::string &key) {
                auto it = n.attributes.find(key);
                if (it == n.attributes.end())
                    throw py::key_error("Key '" + key + "' not found");
                return it->second;
            })
        .def("__repr__",
            [](const chronograph::Node &n){
                return "<Node id='" + n.id + "'>";
            })
        ;
    py::class_<chronograph::Edge>(m, "Edge")
        .def_readwrite("id", &chronograph::Edge::id)
        .def_readwrite("from",&chronograph::Edge::from)
        .def_readwrite("to", &chronograph::Edge::to)
        .def_readwrite("attributes", &chronograph::Edge::attributes)
        .def_readwrite("created_timestamp", &chronograph::Edge::createdTimestamp)
        ;

    // --- Graph ---
    py::class_<Graph>(m, "Graph")
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
        .def("get_nodes", &Graph::getNodes)
        .def("get_edges", &Graph::getEdges)
        .def("get_outgoing", &Graph::getOutgoing)
        ;

    // --- Snapshot ---
    py::class_<Snapshot>(m, "Snapshot")
        .def(py::init<const Graph&, std::int64_t>())
        .def("get_nodes", &Snapshot::getNodes)
        .def("get_edges", &Snapshot::getEdges)
        .def("get_outgoing", &Snapshot::getOutgoing)
        .def("get_incoming", &Snapshot::getIncoming)
        ;

    // --- Algorithms (free functions) ---
    m.def("is_reachable", &graph::algorithms::isReachable,
          py::arg("g"), py::arg("start"), py::arg("target"));
    m.def("shortest_path", &graph::algorithms::shortestPath,
          py::arg("g"), py::arg("start"), py::arg("target"));
    m.def("weakly_connected_components", &graph::algorithms::weaklyConnectedComponents);
    m.def("strongly_connected_components", &graph::algorithms::stronglyConnectedComponents);
    m.def("has_cycle", &graph::algorithms::hasCycle);
    m.def("topological_sort", &graph::algorithms::topologicalSort);

    // --- Repository ---

    // Bind MergePolicy enum
    py::enum_<MergePolicy>(m, "MergePolicy")
        .value("OURS", MergePolicy::OURS)
        .value("THEIRS", MergePolicy::THEIRS)
        .value("ATTRIBUTE_UNION", MergePolicy::ATTRIBUTE_UNION)
        .value("INTERACTIVE", MergePolicy::INTERACTIVE)
        .export_values();

    py::class_<Repository>(m, "Repository")
        .def_static("init", &Repository::init, py::arg("root_branch") = "main")
        .def("add_node", &Repository::addNode)
        .def("add_edge", &Repository::addEdge)
        .def("commit", &Repository::commit, py::arg("message") = "")
        .def("branch", &Repository::branch)
        .def("checkout", &Repository::checkout)
        .def("list_branches", &Repository::listBranches)
        .def("list_commits", &Repository::listCommits)
        .def("merge", &Repository::merge, py::arg("branch"), py::arg("policy"))
        .def("graph", &Repository::graph, py::return_value_policy::reference)
        ;
}