import chronograph

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
