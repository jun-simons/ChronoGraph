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
