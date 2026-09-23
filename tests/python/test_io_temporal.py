import pytest
import chronograph
from chronograph import algorithms as alg, temporal


def make_repo():
    repo = chronograph.Repository.init("main")
    repo.add_node("A", {"v": "1"}, 1)
    repo.commit("add A")
    repo.branch("dev"); repo.checkout("dev")
    repo.add_node("B", {}, 2)
    repo.add_edge("e", "A", "B", {}, 3)
    repo.commit("add B")
    repo.update_node("A", {"v": "2"}, 4)  # left uncommitted
    return repo


def test_repository_save_load_roundtrip(tmp_path):
    repo = make_repo()
    path = tmp_path / "repo.json"
    chronograph.save_repository(repo, path)
    loaded = chronograph.load_repository(path)

    assert loaded.current_branch() == "dev"
    assert loaded.has_uncommitted_changes()
    assert loaded.graph().get_nodes()["A"]["v"] == "2"
    assert chronograph.repository_to_json(loaded) == path.read_text()

    with pytest.raises(chronograph.FormatError):
        chronograph.repository_from_json('{"format": "chronograph.graph", "version": 1}')


def test_repo_refs_and_diff():
    repo = make_repo()
    repo.commit("bump A")
    d = repo.diff("main", "dev")
    assert [n.id for n in d.nodes_added] == ["B"]
    assert [e.id for e in d.edges_added] == ["e"]
    assert repo.graph_at("main").has_node("A")
    assert not repo.graph_at("main").has_node("B")


def test_temporal_queries_and_snapshot_algorithms():
    g = chronograph.graph_from_json(chronograph.graph_to_json(make_repo().graph()))

    assert [e.type for e in temporal.node_history(g, "A")] == [
        chronograph.EventType.ADD_NODE, chronograph.EventType.UPDATE_NODE]
    assert temporal.node_at(g, "A", 3)["v"] == "1"
    assert temporal.node_at(g, "B", 1) is None
    assert temporal.change_times(g, temporal.TimeRange(start=2, end=3)) == [2, 3]

    # algorithms accept Snapshots as well as Graphs
    assert not alg.is_reachable(chronograph.Snapshot(g, 2), "A", "B")
    assert alg.is_reachable(g, "A", "B")
