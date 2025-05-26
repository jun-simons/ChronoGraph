import chronograph_module

def test_graph_basic_nodes_edges():
    g = chronograph_module.Graph()
    # no nodes initially
    assert g.get_nodes() == {}
    g.add_node("A", {"role":"admin"}, 1)
    g.add_node("B", {"role":"user"}, 2)
    nodes = g.get_nodes()
    assert "A" in nodes and nodes["A"]["role"] == "admin"
    # add an edge and check outgoing
    g.add_edge("e1", "A", "B", {}, 3)
    out = g.get_outgoing()
    assert out["A"] == ["e1"]
    assert "B" not in out or out["B"] == []
