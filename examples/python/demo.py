#!/usr/bin/env python3
# examples/python/demo.py

import chronograph
from chronograph import algorithms as alg
from chronograph import MergePolicy

def graph_example():
    print("=== Graph & Algorithms Example ===")
    g = chronograph.Graph()
    # add three nodes
    g.add_node("A", {"label":"start"}, 1)
    g.add_node("B", {"label":"middle"}, 2)
    g.add_node("C", {"label":"end"}, 3)
    # connect: A->B and B->C
    g.add_edge("e1", "A", "B", {"w":"1"}, 4)
    g.add_edge("e2", "B", "C", {"w":"2"}, 5)

    print("Nodes:", list(g.get_nodes().keys()))
    print("Edges:", list(g.get_edges().keys()))

    # reachability
    print("A → C reachable?", alg.is_reachable(g, "A", "C"))

    # shortest path
    path = alg.shortest_path(g, "A", "C")
    print("Shortest A→C:", " -> ".join(path))

    # snapshot at time=4 (before e2)
    snap = chronograph.Snapshot(g, timestamp=4)
    print("Snapshot@4 edges:", list(snap.get_edges().keys()))

def repo_example():
    print("\n=== Repository & Branching Example ===")
    repo = chronograph.Repository.init("main")
    # create node X and commit
    repo.add_node("X", {"val":"1"}, 10)
    c1 = repo.commit("add X")
    print("Committed on main:", c1)

    # branch off and add Y
    repo.branch("dev")
    repo.checkout("dev")
    repo.add_node("Y", {"val":"2"}, 20)
    c2 = repo.commit("add Y")
    print("Committed on dev :", c2)

    # check isolation
    repo.checkout("main")
    print("Main nodes:", list(repo.graph().get_nodes().keys()))
    repo.checkout("dev")
    print("Dev  nodes:", list(repo.graph().get_nodes().keys()))

    # merge dev back to main
    repo.checkout("main")
    merge_res = repo.merge("dev", policy=MergePolicy.OURS)
    print("Merged commit:", merge_res.merge_commit_id)
    print("Conflicts:", merge_res.conflicts)
    print("After merge, main nodes:", list(repo.graph().get_nodes().keys()))

if __name__ == "__main__":
    graph_example()
    repo_example()
