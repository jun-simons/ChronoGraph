// tests/test_Merge.cpp

#include <chronograph/repo/Repository.h>
#include <chronograph/graph/Snapshot.h>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <stdexcept>

using namespace chronograph;

TEST(Merge, FastForward) {
    // Setup: on main add A then branch dev and add B
    auto repo = Repository::init("main");
    repo.addNode("A", {{"val","1"}}, /*ts=*/1);
    auto c1 = repo.commit("add A");

    repo.branch("dev");
    repo.checkout("dev");
    repo.addNode("B", {{"val","2"}}, /*ts=*/2);
    auto c2 = repo.commit("add B");

    // Back to main, merge dev
    repo.checkout("main");
    auto result = repo.merge("dev", MergePolicy::OURS);

    // Fast-forward merge should just move main to c2
    EXPECT_EQ(result.mergeCommitId, c2);
    EXPECT_TRUE(result.conflicts.empty());

    // Graph now has A and B
    {
        const auto& nodes = repo.graph().getNodes();
        EXPECT_EQ(nodes.size(), 2u);
        EXPECT_TRUE(nodes.count("A"));
        EXPECT_TRUE(nodes.count("B"));
    }
    // Snapshot also reflects both
    {
        Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
        EXPECT_EQ(s.getNodes().size(), 2u);
        EXPECT_TRUE(s.getNodes().count("A"));
        EXPECT_TRUE(s.getNodes().count("B"));
    }
}

TEST(Merge, ThreeWayNoConflict) {
    // Setup: add A on main, branch dev adds B, then branch feature adds C
    auto repo = Repository::init("main");
    repo.addNode("A", {{"val","1"}}, /*ts=*/1);
    auto c1 = repo.commit("add A");

    // dev
    repo.branch("dev");
    repo.checkout("dev");
    repo.addNode("B", {{"val","2"}}, /*ts=*/2);
    auto c2 = repo.commit("add B");

    // feature from main
    repo.checkout("main");
    repo.branch("feat");
    repo.checkout("feat");
    repo.addNode("C", {{"val","3"}}, /*ts=*/3);
    auto c3 = repo.commit("add C");

    // back to main, which is still at c1
    repo.checkout("main");
    EXPECT_EQ(repo.graph().getNodes().size(), 1u);

    // Merge dev into main => fast-forward to c2
    auto r1 = repo.merge("dev", MergePolicy::OURS);
    EXPECT_EQ(r1.mergeCommitId, c2);
    EXPECT_TRUE(r1.conflicts.empty());
    EXPECT_EQ(repo.graph().getNodes().size(), 2u);

    // Now merge feat into main => three-way merge (no ancestor relationship)
    auto r2 = repo.merge("feat", MergePolicy::OURS);
    // mergeCommitId should be new and not equal c3
    EXPECT_NE(r2.mergeCommitId, c3);
    EXPECT_NE(r2.mergeCommitId, c2);
    EXPECT_TRUE(r2.conflicts.empty());

    // Graph now has A, B, C
    {
        const auto& nodes = repo.graph().getNodes();
        EXPECT_EQ(nodes.size(), 3u);
        EXPECT_TRUE(nodes.count("A"));
        EXPECT_TRUE(nodes.count("B"));
        EXPECT_TRUE(nodes.count("C"));
    }
    // And snapshot agrees
    {
        Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
        EXPECT_EQ(s.getNodes().size(), 3u);
    }
}

TEST(RepositoryMergeCommitParents, OnlyThreeWayGetsTwoParents) {
    auto repo = Repository::init("main");

    // Base: add A on main
    repo.addNode("A", {{"v","1"}}, /*ts=*/1);
    auto c1 = repo.commit("add A");

    // dev branch off main, adds B
    repo.branch("dev");
    repo.checkout("dev");
    repo.addNode("B", {{"v","2"}}, /*ts=*/2);
    auto c2 = repo.commit("add B");

    // feature branch off main (still at c1), adds C
    repo.checkout("main");
    repo.branch("feat");
    repo.checkout("feat");
    repo.addNode("C", {{"v","3"}}, /*ts=*/3);
    auto c3 = repo.commit("add C");

    // Merge dev into main -> fast-forward to c2 (no merge commit)
    repo.checkout("main");
    auto r1 = repo.merge("dev", MergePolicy::OURS);
    EXPECT_EQ(r1.mergeCommitId, c2);
    EXPECT_TRUE(r1.conflicts.empty());
    {
      // Should still see A,B only
      auto nodes = repo.graph().getNodes();
      EXPECT_EQ(nodes.size(), 2u);
      EXPECT_TRUE(nodes.count("A"));
      EXPECT_TRUE(nodes.count("B"));
    }

    // Merge feat into main -> true three-way; expect a new merge commit
    auto r2 = repo.merge("feat", MergePolicy::OURS);
    EXPECT_NE(r2.mergeCommitId, c3);
    EXPECT_NE(r2.mergeCommitId, c2);
    EXPECT_TRUE(r2.conflicts.empty());

    // Now inspect the final commit (tip of main)
    auto commits = repo.listCommits("main");
    const Commit& mergeC = commits.back();
    ASSERT_EQ(mergeC.parents.size(), 2u);
    // parents must be c2 and c3 (in any order)
    EXPECT_TRUE(
      (mergeC.parents[0] == c2 && mergeC.parents[1] == c3) ||
      (mergeC.parents[0] == c3 && mergeC.parents[1] == c2)
    );

    // And the graph has A,B,C
    {
      auto nodes = repo.graph().getNodes();
      EXPECT_EQ(nodes.size(), 3u);
      EXPECT_TRUE(nodes.count("A"));
      EXPECT_TRUE(nodes.count("B"));
      EXPECT_TRUE(nodes.count("C"));
    }
}

TEST(Merge, FastForwardThroughSecondParent) {
    // feat is an ancestor of other's tip only via a merge commit's second
    // parent, so the fast-forward can't walk first parents back to it
    auto repo = Repository::init("main");
    repo.addNode("A", {}, 1);
    repo.commit("A");
    repo.branch("feat");
    repo.branch("other");

    repo.checkout("feat");
    repo.addNode("F", {}, 2);
    repo.commit("F");

    repo.checkout("other");
    repo.addNode("O", {}, 3);
    repo.addEdge("e", "A", "O", {}, 3);
    repo.commit("O");
    auto m = repo.merge("feat");  // three-way: parents {O, F}

    repo.checkout("feat");
    auto r = repo.merge("other");
    EXPECT_EQ(r.mergeCommitId, m.mergeCommitId);

    const auto& nodes = repo.graph().getNodes();
    EXPECT_EQ(nodes.size(), 3u);
    EXPECT_TRUE(nodes.count("F") && nodes.count("O"));
    EXPECT_EQ(repo.graph().getEdges().at("e").createdTimestamp, 3);

    // Merging an ancestor back in is a no-op
    EXPECT_EQ(repo.merge("main").mergeCommitId, m.mergeCommitId);
}

// ─────────────────────────────────────────────────────────────────────────────
// Three-way merges, conflicts and policies
// ─────────────────────────────────────────────────────────────────────────────

namespace {

using Change = std::function<void(Repository&)>;

// main: base -> ours;  feat: base -> theirs;  ends checked out on main
Repository diverge(const Change& base, const Change& ours, const Change& theirs) {
    auto repo = Repository::init("main");
    base(repo);
    repo.commit("base");
    repo.branch("feat");
    ours(repo);
    repo.commit("ours");
    repo.checkout("feat");
    theirs(repo);
    repo.commit("theirs");
    repo.checkout("main");
    return repo;
}

// Merged graphs must be well-formed, and the merge commit's history must
// reproduce the working graph exactly
void expectConsistent(const Repository& repo) {
    const Graph& g = repo.graph();
    for (const auto& [id, e] : g.getEdges()) {
        EXPECT_TRUE(g.hasNode(e.from) && g.hasNode(e.to)) << "dangling edge " << id;
    }
    Graph replayed = repo.graphAt(repo.headCommit());
    EXPECT_EQ(replayed.getNodes(), g.getNodes());
    EXPECT_EQ(replayed.getEdges(), g.getEdges());
}

std::string attr(const Repository& repo, const std::string& node, const std::string& key) {
    return repo.graph().getNodes().at(node).attributes.at(key);
}

}  // namespace

TEST(ThreeWayMerge, CombinesIndependentChangesAndKeepsTheirTimestamps) {
    auto repo = diverge(
        [](Repository& r) {
            r.addNode("n", {{"a","0"}, {"b","0"}}, 1);
            r.addNode("gone", {}, 1);
        },
        [](Repository& r) {                        // ours
            r.updateNode("n", {{"a","ours"}}, 10);
            r.delNode("gone", 11);
        },
        [](Repository& r) {                        // theirs
            r.updateNode("n", {{"b","theirs"}}, 5);
            r.addNode("m", {}, 6);
            r.addEdge("e", "n", "m", {}, 7);
        });

    auto result = repo.merge("feat", MergePolicy::OURS);
    EXPECT_TRUE(result.conflicts.empty());
    EXPECT_EQ(repo.getCommit(result.mergeCommitId).parents.size(), 2u);

    EXPECT_EQ(attr(repo, "n", "a"), "ours");
    EXPECT_EQ(attr(repo, "n", "b"), "theirs");
    EXPECT_FALSE(repo.graph().hasNode("gone"));
    EXPECT_EQ(repo.graph().getEdges().at("e").createdTimestamp, 7);
    expectConsistent(repo);

    // Their changes keep their own times: m exists at t=6, before our edits
    Snapshot at6(repo.graph(), 6);
    EXPECT_TRUE(at6.hasNode("m"));
    EXPECT_FALSE(at6.hasEdge("e"));
}

TEST(ThreeWayMerge, PoliciesSettleAttributeClashes) {
    auto make = [] {
        return diverge(
            [](Repository& r) { r.addNode("n", {{"k","base"}}, 1); },
            [](Repository& r) { r.updateNode("n", {{"k","ours"}, {"x","1"}}, 2); },
            [](Repository& r) { r.updateNode("n", {{"k","theirs"}, {"y","1"}}, 3); });
    };
    const std::map<MergePolicy, std::string> expected{
        {MergePolicy::OURS, "ours"}, {MergePolicy::THEIRS, "theirs"},
        {MergePolicy::ATTRIBUTE_UNION, "ours"}};

    for (const auto& [policy, value] : expected) {
        auto repo = make();
        auto result = repo.merge("feat", policy);
        ASSERT_EQ(result.conflicts.size(), 1u);
        const Conflict& c = result.conflicts[0];
        EXPECT_EQ(c.kind, Conflict::UPDATE_UPDATE);
        EXPECT_EQ(c.entity, Conflict::NODE);
        EXPECT_EQ(c.keys, std::vector<std::string>{"k"});
        EXPECT_EQ(c.theirs->attributes.at("k"), "theirs");

        // Only the clashing key follows the policy; the rest merges
        EXPECT_EQ(attr(repo, "n", "k"), value);
        EXPECT_EQ(attr(repo, "n", "x"), "1");
        EXPECT_EQ(attr(repo, "n", "y"), "1");
        expectConsistent(repo);
    }
}

TEST(ThreeWayMerge, DeleteVersusUpdate) {
    auto make = [] {
        return diverge(
            [](Repository& r) { r.addNode("n", {{"k","0"}}, 1); },
            [](Repository& r) { r.delNode("n", 2); },
            [](Repository& r) { r.updateNode("n", {{"k","1"}}, 3); });
    };
    const std::map<MergePolicy, bool> survives{
        {MergePolicy::OURS, false}, {MergePolicy::THEIRS, true},
        {MergePolicy::ATTRIBUTE_UNION, true}};

    for (const auto& [policy, alive] : survives) {
        auto repo = make();
        auto result = repo.merge("feat", policy);
        ASSERT_EQ(result.conflicts.size(), 1u);
        EXPECT_EQ(result.conflicts[0].kind, Conflict::DEL_UPDATE);
        EXPECT_FALSE(result.conflicts[0].ours.has_value());
        EXPECT_EQ(repo.graph().hasNode("n"), alive);
        if (alive) EXPECT_EQ(attr(repo, "n", "k"), "1");
        expectConsistent(repo);
    }
}

TEST(ThreeWayMerge, EdgesToADeletedNodeFollowIt) {
    // We delete n; they add an edge into it
    auto make = [] {
        return diverge(
            [](Repository& r) { r.addNode("n", {}, 1); r.addNode("x", {}, 1); },
            [](Repository& r) { r.delNode("n", 2); },
            [](Repository& r) { r.addEdge("e", "x", "n", {{"w","1"}}, 3); });
    };

    auto ours = make();
    auto r1 = ours.merge("feat", MergePolicy::OURS);
    ASSERT_EQ(r1.conflicts.size(), 1u);
    const Conflict& c = r1.conflicts[0];
    EXPECT_EQ(c.kind, Conflict::DEL_UPDATE);
    EXPECT_EQ(c.id, "n");
    ASSERT_EQ(c.dependentEdges.size(), 1u);
    EXPECT_EQ(c.dependentEdges[0].id, "e");
    EXPECT_FALSE(ours.graph().hasNode("n"));
    EXPECT_FALSE(ours.graph().hasEdge("e"));
    expectConsistent(ours);

    auto theirs = make();
    theirs.merge("feat", MergePolicy::THEIRS);
    EXPECT_TRUE(theirs.graph().hasNode("n"));
    EXPECT_EQ(theirs.graph().getEdges().at("e").attributes.at("w"), "1");
    expectConsistent(theirs);
}

TEST(ThreeWayMerge, AddAddConflictsOnlyWhenDifferent) {
    auto repo = diverge(
        [](Repository& r) { r.addNode("a", {}, 1); r.addNode("b", {}, 1); r.addNode("c", {}, 1); },
        [](Repository& r) {
            r.addNode("same", {{"v","1"}}, 2);
            r.addNode("clash", {{"v","ours"}, {"o","1"}}, 2);
            r.addEdge("e", "a", "b", {}, 2);
        },
        [](Repository& r) {
            r.addNode("same", {{"v","1"}}, 3);
            r.addNode("clash", {{"v","theirs"}, {"t","1"}}, 3);
            r.addEdge("e", "a", "c", {}, 3);
        });

    auto result = repo.merge("feat", MergePolicy::THEIRS);
    ASSERT_EQ(result.conflicts.size(), 2u);  // clash (node) and e (edge); not `same`
    for (const auto& c : result.conflicts) {
        EXPECT_EQ(c.kind, Conflict::ADD_ADD);
        if (c.entity == Conflict::EDGE) EXPECT_TRUE(c.endpoints);
        else EXPECT_EQ(c.keys, std::vector<std::string>{"v"});
    }
    EXPECT_EQ(attr(repo, "clash", "v"), "theirs");
    EXPECT_EQ(attr(repo, "clash", "o"), "1");  // non-clashing keys from both
    EXPECT_EQ(repo.graph().getEdges().at("e").to, "c");
    expectConsistent(repo);
}

TEST(ThreeWayMerge, UsesLowestCommonAncestor) {
    // feat already merged main once; the next merge must use that point as
    // its base, not the original fork, or n's earlier change looks like a clash
    auto repo = Repository::init("main");
    repo.addNode("n", {{"k","1"}}, 1);
    repo.addNode("m", {}, 1);
    repo.commit("c1");
    repo.branch("feat");
    repo.updateNode("n", {{"k","2"}}, 2);
    repo.commit("c2");

    repo.checkout("feat");
    repo.updateNode("m", {{"x","1"}}, 3);
    repo.commit("d1");
    ASSERT_TRUE(repo.merge("main").conflicts.empty());  // feat now has k=2
    repo.updateNode("m", {{"x","2"}}, 4);
    repo.commit("d2");

    repo.checkout("main");
    repo.updateNode("n", {{"k","3"}}, 5);
    repo.commit("c3");

    auto result = repo.merge("feat", MergePolicy::INTERACTIVE);
    EXPECT_TRUE(result.conflicts.empty());
    EXPECT_FALSE(result.mergeCommitId.empty());
    EXPECT_EQ(attr(repo, "n", "k"), "3");
    EXPECT_EQ(attr(repo, "m", "x"), "2");
    expectConsistent(repo);
}

TEST(InteractiveMerge, ResolveEditAndCommit) {
    auto repo = diverge(
        [](Repository& r) { r.addNode("p", {{"k","0"}}, 1); r.addNode("q", {{"k","0"}}, 1); },
        [](Repository& r) {
            r.updateNode("p", {{"k","ours"}}, 2);
            r.updateNode("q", {{"k","ours"}}, 2);
        },
        [](Repository& r) {
            r.updateNode("p", {{"k","theirs"}}, 3);
            r.updateNode("q", {{"k","theirs"}}, 3);
            r.addNode("clean", {}, 3);
        });
    const auto oursTip = repo.headCommit();

    auto result = repo.merge("feat", MergePolicy::INTERACTIVE);
    EXPECT_TRUE(result.mergeCommitId.empty());
    ASSERT_EQ(result.conflicts.size(), 2u);
    EXPECT_TRUE(repo.isMerging());
    EXPECT_TRUE(repo.graph().hasNode("clean"));  // clean changes already applied
    EXPECT_EQ(attr(repo, "p", "k"), "ours");     // conflicts start as ours

    // No leaving, merging again, committing or saving mid-merge
    EXPECT_THROW(repo.checkout("feat"), std::runtime_error);
    EXPECT_THROW(repo.merge("feat"), std::runtime_error);
    EXPECT_THROW(repo.commit(), std::runtime_error);
    EXPECT_THROW(repo.exportData(), std::runtime_error);

    repo.resolveConflict(result.conflicts[0], Resolution::THEIRS);  // p
    repo.updateNode("q", {{"k","hand-merged"}}, 4);
    repo.resolveConflict(result.conflicts[1], Resolution::MANUAL);  // q
    EXPECT_TRUE(repo.mergeConflicts().empty());
    EXPECT_THROW(repo.resolveConflict(result.conflicts[0], Resolution::OURS),
                 std::invalid_argument);  // already resolved

    auto mergeId = repo.commit();
    EXPECT_FALSE(repo.isMerging());
    EXPECT_EQ(repo.getCommit(mergeId).parents,
              (std::vector<std::string>{oursTip, repo.listCommits("feat").back().id}));
    EXPECT_EQ(attr(repo, "p", "k"), "theirs");
    EXPECT_EQ(attr(repo, "q", "k"), "hand-merged");
    expectConsistent(repo);
}

TEST(InteractiveMerge, AbortRestoresHead) {
    auto repo = diverge(
        [](Repository& r) { r.addNode("n", {{"k","0"}}, 1); },
        [](Repository& r) { r.updateNode("n", {{"k","ours"}}, 2); },
        [](Repository& r) { r.updateNode("n", {{"k","theirs"}}, 3); r.addNode("m", {}, 3); });
    const auto head = repo.headCommit();

    repo.merge("feat", MergePolicy::INTERACTIVE);
    ASSERT_TRUE(repo.isMerging());
    repo.abortMerge();

    EXPECT_FALSE(repo.isMerging());
    EXPECT_FALSE(repo.hasUncommittedChanges());
    EXPECT_EQ(repo.headCommit(), head);
    EXPECT_FALSE(repo.graph().hasNode("m"));
    EXPECT_THROW(repo.abortMerge(), std::runtime_error);
    repo.checkout("feat");  // free to move again
}
