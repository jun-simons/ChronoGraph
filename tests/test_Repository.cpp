// tests/test_Repository.cpp

#include <chronograph/repo/Repository.h>
#include <chronograph/graph/Snapshot.h>
#include <gtest/gtest.h>
#include <limits>
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


TEST(RepositoryBranchIsolation, MultiBranchWithSnapshots) {
    auto repo = Repository::init("main");

    // Commit a node on main at ts=1
    repo.addNode("a", {{"val","1"}}, /*ts=*/1);
    auto c1 = repo.commit("add a");

    // Create & switch to dev branch
    repo.branch("dev");
    repo.checkout("dev");
    // Commit b on dev at ts=2
    repo.addNode("b", {{"val","2"}}, /*ts=*/2);
    auto c2 = repo.commit("add b");
    // Commit c on dev at ts=3
    repo.addNode("c", {{"val","3"}}, /*ts=*/3);
    auto c3 = repo.commit("add c");

    // --- main should only have a ---
    repo.checkout("main");
    {
      // working-graph check
      const auto& nodes = repo.graph().getNodes();
      EXPECT_EQ(nodes.size(), 1u);
      EXPECT_TRUE(nodes.count("a"));
      EXPECT_FALSE(nodes.count("b"));
      EXPECT_FALSE(nodes.count("c"));

      // snapshot check
      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& sn = s.getNodes();
      EXPECT_EQ(sn.size(), 1u);
      EXPECT_TRUE(sn.count("a"));
    }

    // --- dev should have a,b,c ---
    repo.checkout("dev");
    {
      const auto& nodes = repo.graph().getNodes();
      EXPECT_EQ(nodes.size(), 3u);
      EXPECT_TRUE(nodes.count("a"));
      EXPECT_TRUE(nodes.count("b"));
      EXPECT_TRUE(nodes.count("c"));

      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& sn = s.getNodes();
      EXPECT_EQ(sn.size(), 3u);
      EXPECT_TRUE(sn.count("a"));
      EXPECT_TRUE(sn.count("b"));
      EXPECT_TRUE(sn.count("c"));
    }

    // Create & switch to feature from dev
    repo.branch("feature");
    repo.checkout("feature");
    // Commit edge a->b on feature at ts=4
    repo.addEdge("e_ab", "a", "b", {{"w","10"}}, /*ts=*/4);
    auto c4 = repo.commit("add edge ab");

    // --- feature should have a,b,c and e_ab ---
    {
      const auto& nodes = repo.graph().getNodes();
      const auto& edges = repo.graph().getEdges();
      EXPECT_EQ(nodes.size(), 3u);
      EXPECT_EQ(edges.size(), 1u);
      EXPECT_TRUE(edges.count("e_ab"));

      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& sn = s.getNodes();
      const auto& se = s.getEdges();
      EXPECT_EQ(sn.size(), 3u);
      EXPECT_EQ(se.size(), 1u);
      EXPECT_TRUE(se.count("e_ab"));
    }

    // --- dev still has no e_ab ---
    repo.checkout("dev");
    {
      const auto& edges = repo.graph().getEdges();
      EXPECT_TRUE(edges.empty());

      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& se = s.getEdges();
      EXPECT_TRUE(se.empty());
    }

    // --- main still only a ---
    repo.checkout("main");
    {
      const auto& nodes = repo.graph().getNodes();
      const auto& edges = repo.graph().getEdges();
      EXPECT_EQ(nodes.size(), 1u);
      EXPECT_TRUE(nodes.count("a"));
      EXPECT_TRUE(edges.empty());

      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& sn = s.getNodes();
      const auto& se = s.getEdges();
      EXPECT_EQ(sn.size(), 1u);
      EXPECT_TRUE(sn.count("a"));
      EXPECT_TRUE(se.empty());
    }

    // --- check feature again, it remains correct ---
    repo.checkout("feature");
    {
      const auto& nodes = repo.graph().getNodes();
      const auto& edges = repo.graph().getEdges();
      EXPECT_EQ(nodes.size(), 3u);
      EXPECT_EQ(edges.size(), 1u);
      EXPECT_TRUE(edges.count("e_ab"));

      Snapshot s(repo.graph(), std::numeric_limits<int64_t>::max());
      const auto& sn = s.getNodes();
      const auto& se = s.getEdges();
      EXPECT_EQ(sn.size(), 3u);
      EXPECT_EQ(se.size(), 1u);
      EXPECT_TRUE(se.count("e_ab"));
    }
}

TEST(RepositoryNoChangeCommit, IdUnchanged) {
    auto repo = Repository::init("main");
    // initial commit is root; two no-op commits should return same ID
    auto rootId = repo.commit("first no-op");
    auto sameId = repo.commit("second no-op");
    EXPECT_EQ(rootId, sameId);

    // listCommits should only have the root commit
    auto commits = repo.listCommits("main");
    ASSERT_EQ(commits.size(), 1u);
    EXPECT_EQ(commits[0].id, rootId);
}

TEST(RepositoryUpdateOperations, NodeEdgeUpdate) {
    auto repo = Repository::init("main");
    // Add a node and commit
    repo.addNode("n", {{"x","old"}}, /*ts=*/1);
    auto c1 = repo.commit("add n");

    // Update the node and commit
    repo.updateNode("n", {{"x","new"}, {"y","v"}}, /*ts=*/2);
    auto c2 = repo.commit("update n");

    // On main, the graph should reflect the updated attributes
    {
        const auto& nodes = repo.graph().getNodes();
        ASSERT_EQ(nodes.size(), 1u);
        const auto& n = nodes.at("n");
        EXPECT_EQ(n.attributes.at("x"), "new");
        EXPECT_EQ(n.attributes.at("y"), "v");
    }

    // Add a second node and an edge, then update the edge
    repo.addNode("m", {}, /*ts=*/3);
    repo.addEdge("e", "n", "m", {{"wt","5"}}, /*ts=*/4);
    auto c3 = repo.commit("add edge");

    repo.updateEdge("e", {{"wt","15"}, {"label","flow"}}, /*ts=*/5);
    auto c4 = repo.commit("update edge");

    // Edge should exist with merged attributes
    {
        const auto& edges = repo.graph().getEdges();
        ASSERT_EQ(edges.size(), 1u);
        const auto& e = edges.at("e");
        EXPECT_EQ(e.attributes.at("wt"),    "15");
        EXPECT_EQ(e.attributes.at("label"), "flow");
    }

    // listCommits should be: root, c1, c2, c3, c4
    auto commits = repo.listCommits("main");
    ASSERT_EQ(commits.size(), 5u);
    EXPECT_EQ(commits[1].id, c1);
    EXPECT_EQ(commits[2].id, c2);
    EXPECT_EQ(commits[3].id, c3);
    EXPECT_EQ(commits[4].id, c4);
}

TEST(RepositoryCommitGraph, ParentsAndChildren) {
  // 1) Build a repo with:
  //    – root (implicit)
  //    – c1: add A               (parent = root)
  //    – dev → c2: add B         (parent = c1)
  //    – feat → c3: add C        (parent = c1)
  //    – main ← dev (fast-forward to c2)
  //    – main ← feat (three-way merge c4 with parents c2,c3)
  auto repo = Repository::init("main");

  // c1
  repo.addNode("A", {{"v","1"}}, 1);
  auto c1 = repo.commit("add A");

  // c2 on dev
  repo.branch("dev");
  repo.checkout("dev");
  repo.addNode("B", {{"v","2"}}, 2);
  auto c2 = repo.commit("add B");

  // c3 on feat (branched off main at c1)
  repo.checkout("main");
  repo.branch("feat");
  repo.checkout("feat");
  repo.addNode("C", {{"v","3"}}, 3);
  auto c3 = repo.commit("add C");

  // fast-forward main to dev (→ c2)
  repo.checkout("main");
  auto r1 = repo.merge("dev", MergePolicy::OURS);
  EXPECT_EQ(r1.mergeCommitId, c2);
  EXPECT_TRUE(r1.conflicts.empty());

  // three-way merge feat into main → c4
  auto r2 = repo.merge("feat", MergePolicy::OURS);
  // merge-commit must not equal c2 or c3
  ASSERT_NE(r2.mergeCommitId, c2);
  ASSERT_NE(r2.mergeCommitId, c3);
  EXPECT_TRUE(r2.conflicts.empty());
  auto c4 = r2.mergeCommitId;

  // 2) Pull out the chain in topological order via listCommits
  auto commits = repo.listCommits("main");
  ASSERT_EQ(commits.size(), 5u);
  auto c0 = commits[0].id;  // root
  EXPECT_EQ(commits[1].id, c1);
  EXPECT_EQ(commits[2].id, c2);
  EXPECT_EQ(commits[3].id, c3);
  EXPECT_EQ(commits[4].id, c4);

  // 3) Get the DAG
  auto dag = repo.getCommitGraph();

  // 4) Parents
  EXPECT_TRUE(dag.parents.at(c0).empty());
  EXPECT_EQ(dag.parents.at(c1), std::vector<std::string>{c0});
  EXPECT_EQ(dag.parents.at(c2), std::vector<std::string>{c1});
  EXPECT_EQ(dag.parents.at(c3), std::vector<std::string>{c1});

  auto p4 = dag.parents.at(c4);
  ASSERT_EQ(p4.size(), 2u);
  EXPECT_TRUE((p4[0]==c2 && p4[1]==c3) || (p4[0]==c3 && p4[1]==c2));

  // 5) Children
  EXPECT_EQ(dag.children.at(c0), std::vector<std::string>{c1});

  auto ch1 = dag.children.at(c1);
  ASSERT_EQ(ch1.size(), 2u);
  EXPECT_TRUE((ch1[0]==c2 && ch1[1]==c3) || (ch1[0]==c3 && ch1[1]==c2));

  EXPECT_EQ(dag.children.at(c2), std::vector<std::string>{c4});
  EXPECT_EQ(dag.children.at(c3), std::vector<std::string>{c4});
  EXPECT_TRUE(dag.children.at(c4).empty());
}