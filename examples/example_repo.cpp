#include <iostream>
#include <limits>

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Snapshot.h>
#include <chronograph/repo/Repository.h>
#include <chronograph/repo/utils/printUtils.h>

using namespace chronograph;

int main() {
    // 1) Initialize a repository with a “main” branch
    Repository repo = Repository::init("main");

    // 2) On main: add node “A” and commit
    repo.addNode("A", {{"color","blue"}}, /*ts=*/1);
    auto c1 = repo.commit("Add A");

    // 3) Create + switch to branch “dev”, add “B” and commit
    repo.branch("dev");
    repo.checkout("dev");
    repo.addNode("B", {{"color","green"}}, /*ts=*/2);
    auto c2 = repo.commit("Add B");

    // 4) Switch back to main, merge dev
    repo.checkout("main");
    auto r1 = repo.merge("dev", MergePolicy::OURS);
    std::cout << "[merge] fast-forwarded to commit: " << r1.mergeCommitId << "\n";

    // 5) Create + switch to branch “feat” off of main, add “C”, commit
    repo.branch("feat");
    repo.checkout("feat");
    repo.addNode("C", {{"color","red"}}, /*ts=*/3);
    auto c3 = repo.commit("Add C");

    // 6) Switch to main and do a three-way merge of feat
    repo.checkout("main");
    auto r2 = repo.merge("feat", MergePolicy::OURS);
    std::cout << "[merge] three-way merge commit: " << r2.mergeCommitId << "\n";

    // 7) Print out the full commit DAG
    std::cout << "\n=== Commit DAG ===\n";
    CommitGraph dag = repo.getCommitGraph();
    printCommitGraph(dag, std::cout);

    // 8) Show the final graph state via workingGraph
    std::cout << "\n=== Final Graph Nodes ===\n";
    for (auto& [id,node] : repo.graph().getNodes()) {
        std::cout << " - " << id 
                  << " (color=" << node.attributes.at("color") << ")\n";
    }

    // 9) Take a snapshot at t=2 
    std::cout << "\n=== Snapshot at 2 ===\n";
    Snapshot snap(repo.graph(), /*ts=*/2);
    for (auto& [id,node] : snap.getNodes()) {
        std::cout << " * " << id 
                  << " (color=" << node.attributes.at("color") << ")\n";
    }

    // 9) Take a snapshot at “now” and show it too
    std::cout << "\n=== Snapshot at ∞ ===\n";
    Snapshot snap2(repo.graph(), std::numeric_limits<int64_t>::max());
    for (auto& [id,node] : snap2.getNodes()) {
        std::cout << " * " << id 
                  << " (color=" << node.attributes.at("color") << ")\n";
    }

    return 0;
}
