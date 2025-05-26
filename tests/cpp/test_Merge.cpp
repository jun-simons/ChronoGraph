// tests/test_Merge.cpp

#include <chronograph/repo/Repository.h>
#include <chronograph/graph/Snapshot.h>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <string>
#include <algorithm>

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