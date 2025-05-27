import chronograph

def test_graph_basic_nodes_edges():
    g = chronograph.Graph()
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

def test_update_and_delete_node():
    g = chronograph.Graph()
    # add
    g.add_node("N1", {"foo": "bar"}, 10)
    assert "N1" in g.get_nodes()
    # update
    g.update_node("N1", {"foo": "baz", "new": "yes"}, 20)
    n = g.get_nodes()["N1"]
    assert n["foo"] == "baz"
    assert n["new"] == "yes"
    # delete
    g.del_node("N1", 30)
    # after deletion it still appears in nodes_ map? TODO: check on this
    assert "N1" not in g.get_nodes() or g.get_nodes()["N1"].id != "N1"

def test_update_and_delete_edge():
    g = chronograph.Graph()
    g.add_node("A", {}, 1)
    g.add_node("B", {}, 2)
    g.add_edge("E1", "A", "B", {"w": "5"}, 3)
    assert "E1" in g.get_edges()
    # update edge attrs
    g.update_edge("E1", {"w": "50", "new": "attr"}, 4)
    e = g.get_edges()["E1"]
    assert e.attributes["w"] == "50"
    assert e.attributes["new"] == "attr"
    # delete
    g.del_edge("E1", 5)
    assert "E1" not in g.get_edges()

def test_graph_snapshot():
    g = chronograph.Graph()
    g.add_node("X", {}, 1)
    g.add_node("Y", {}, 2)
    g.add_edge("E", "X", "Y", {}, 3)
    # snapshot at time=2 (before edge)
    snap = chronograph.Snapshot(g, 2)
    assert set(snap.get_nodes()) == {"X", "Y"}
    assert snap.get_edges() == {}  # edge not yet visible
    # snapshot at time=3
    snap2 = chronograph.Snapshot(g, 3)
    assert "E" in snap2.get_edges()