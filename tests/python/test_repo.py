import pytest
import chronograph
from chronograph import MergePolicy

def test_repo_basic_commit_and_branch():
    repo = chronograph.Repository.init("main")
    repo.add_node("X", {}, 1)
    cid1 = repo.commit("add X")
    assert cid1  # some nonempty string

    # new branch
    repo.branch("dev")
    repo.checkout("dev")
    repo.add_node("Y", {}, 2)
    cid2 = repo.commit("add Y")
    # dev should see both X and Y
    nodes_dev = repo.graph().get_nodes()
    assert set(nodes_dev) == {"X", "Y"}

    # back to main
    repo.checkout("main")
    nodes_main = repo.graph().get_nodes()
    assert set(nodes_main) == {"X"}

def test_branch_isolation_and_snapshot():
    repo = chronograph.Repository.init("main")
    repo.add_node("X", {}, 1)
    repo.commit("add X")
    repo.branch("dev")
    repo.checkout("dev")
    repo.add_node("Y", {}, 2)
    repo.commit("add Y")
    # snapshot on dev branch
    snap_dev = chronograph.Snapshot(repo.graph(), 2)
    assert "Y" in snap_dev.get_nodes()
    # back to main
    repo.checkout("main")
    snap_main = chronograph.Snapshot(repo.graph(), 2)
    assert "Y" not in snap_main.get_nodes()

def test_merge_simple_fast_forward():
    repo = chronograph.Repository.init("main")
    repo.add_node("A", {}, 1); repo.commit("A")
    repo.branch("feature"); repo.checkout("feature")
    repo.add_node("B", {}, 2); repo.commit("B")
    # merging feature back into main
    repo.checkout("main")
    res = repo.merge("feature", policy=MergePolicy.OURS)
    # fast‐forward: no conflicts, and graph now includes B
    assert res.conflicts == []
    assert "B" in repo.graph().get_nodes()

def test_commit_history_and_checkout_guard():
    repo = chronograph.Repository.init("main")
    repo.add_node("A", {}, 1)
    cid = repo.commit("add A")
    commits = repo.list_commits("main")
    assert [c.id for c in commits][-1] == cid
    assert commits[-1].message == "add A"
    assert commits[-1].events[0].type == chronograph.EventType.ADD_NODE

    repo.branch("dev"); repo.checkout("dev")
    repo.add_node("B", {}, 2); repo.commit("add B")
    repo.update_node("B", {"k": "v"}, 3)
    assert repo.has_uncommitted_changes()
    with pytest.raises(RuntimeError):
        repo.checkout("main")


def _diverged_repo():
    repo = chronograph.Repository.init("main")
    repo.add_node("n", {"k": "0"}, 1); repo.commit("base")
    repo.branch("feat")
    repo.update_node("n", {"k": "ours", "a": "1"}, 2); repo.commit("ours")
    repo.checkout("feat")
    repo.update_node("n", {"k": "theirs", "b": "1"}, 3); repo.commit("theirs")
    repo.checkout("main")
    return repo


def test_merge_policy_reports_conflicts():
    repo = _diverged_repo()
    res = repo.merge("feat", MergePolicy.THEIRS)
    [c] = res.conflicts
    assert c.kind == chronograph.Conflict.Kind.UPDATE_UPDATE
    assert c.entity == chronograph.Conflict.EntityKind.NODE
    assert c.keys == ["k"]
    assert c.ours.attributes["k"] == "ours"
    assert repo.graph().get_nodes()["n"].attributes == {"k": "theirs", "a": "1", "b": "1"}


def test_interactive_merge_flow():
    repo = _diverged_repo()
    res = repo.merge("feat", MergePolicy.INTERACTIVE)
    assert res.merge_commit_id == "" and repo.is_merging()
    for c in repo.merge_conflicts():
        repo.resolve_conflict(c, chronograph.Resolution.THEIRS)
    merge_id = repo.commit()
    assert not repo.is_merging()
    assert len(repo.get_commit(merge_id).parents) == 2
    assert repo.graph().get_nodes()["n"]["k"] == "theirs"
