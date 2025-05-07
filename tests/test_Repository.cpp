// tests/test_Repository.cpp

#include "chronograph/Repository.h"
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <algorithm>

using namespace chronograph;

TEST(RepositoryBranching, InitAndSingleBranch) {
    auto repo = Repository::init("main");
    auto branches = repo.listBranches();
    ASSERT_EQ(branches.size(), 1u);
    EXPECT_EQ(branches[0], "main");

    // No nodes initially
    EXPECT_TRUE(repo.graph().getNodes().empty());
}

TEST(RepositoryCommitAndCheckout, CommitBranchWorkflow) {
    auto repo = Repository::init("main");

    // 1) On main: add a node and commit
    repo.addNode("n1", {{"name","Node1"}}, /*ts=*/1);
    auto c1 = repo.commit("add n1");
    ASSERT_FALSE(c1.empty());

    // Graph on main has only n1
    {
      auto nodes = repo.graph().getNodes();
      ASSERT_EQ(nodes.size(), 1u);
      EXPECT_TRUE(nodes.count("n1"));
    }

    // 2) Create & switch to a new branch "feature"
    repo.branch("feature");
    auto branches = repo.listBranches();
    EXPECT_NE(std::find(branches.begin(), branches.end(), "feature"), branches.end());

    repo.checkout("feature");
    // On feature: add n2 and commit
    repo.addNode("n2", {{"name","Node2"}}, /*ts=*/2);
    auto c2 = repo.commit("add n2");
    ASSERT_FALSE(c2.empty());

    // Graph on feature now has n1 and n2
    {
      auto nodes = repo.graph().getNodes();
      ASSERT_EQ(nodes.size(), 2u);
      EXPECT_TRUE(nodes.count("n1"));
      EXPECT_TRUE(nodes.count("n2"));
    }

    // 3) Switch back to main
    repo.checkout("main");
    // Graph on main should revert to only n1
    {
      auto nodes = repo.graph().getNodes();
      ASSERT_EQ(nodes.size(), 1u);
      EXPECT_TRUE(nodes.count("n1"));
      EXPECT_FALSE(nodes.count("n2"));
    }
}

TEST(RepositoryListCommits, CommitSequence) {
    auto repo = Repository::init("main");

    // initial root commit has no events
    // 1st real commit: add n1
    repo.addNode("n1", {}, /*ts=*/1);
    auto c1 = repo.commit("first");
    // 2nd commit: update n1
    repo.updateNode("n1", {{"val","v1"}}, /*ts=*/2);
    auto c2 = repo.commit("second");

    // list commits on main (should include root + c1 + c2)
    auto commits = repo.listCommits("main");
    ASSERT_EQ(commits.size(), 3u);
    // commits[0] is the implicit root commit (empty events)
    EXPECT_EQ(commits[1].id, c1);
    EXPECT_EQ(commits[2].id, c2);
}
