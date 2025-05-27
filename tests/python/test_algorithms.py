# tests/python/test_algorithms.py
import chronograph
from chronograph import algorithms as alg

def make_sample_graph():
    g = chronograph.Graph()
    for label in ["A", "B", "C", "D", "E"]:
        g.add_node(label, {}, 1)
    # A->B->C and A->D->E, plus cross link C->E
    g.add_edge("e_ab", "A", "B", {}, 2)
    g.add_edge("e_bc", "B", "C", {}, 3)
    g.add_edge("e_ad", "A", "D", {}, 4)
    g.add_edge("e_de", "D", "E", {}, 5)
    g.add_edge("e_ce", "C", "E", {}, 6)
    return g

def test_reachability_and_shortest():
    g = make_sample_graph()
    assert alg.is_reachable(g, "A", "E")
    # two routes A-B-C-E (3 hops) vs A-D-E (2 hops)
    path = alg.shortest_path(g, "A", "E")
    assert path == ["A", "D", "E"]

def test_time_respecting_reachability():
    g = chronograph.Graph()
    # edges created out of order
    g.add_node("1", {}, 1)
    g.add_node("2", {}, 2)
    g.add_node("3", {}, 3)
    g.add_edge("e12", "1", "2", {}, 5)
    g.add_edge("e23", "2", "3", {}, 4)  # timestamp decreases
    # unweighted reachability ignores time, but time-respecting should fail
    assert alg.is_reachable(g, "1", "3")
    assert not alg.is_time_respecting_reachable(g, "1", "3")

def test_components_and_toposort():
    g = make_sample_graph()
    # treat as undirected
    wcc = alg.weakly_connected_components(g)
    assert any(set(comp) == {"A","B","C","D","E"} for comp in wcc)
    # directed has no cycle
    assert not alg.has_cycle(g)
    topo = alg.topological_sort(g)
    assert topo is not None
    # ensure A comes before B, B before C, etc.
    assert topo.index("A") < topo.index("B") < topo.index("C")
