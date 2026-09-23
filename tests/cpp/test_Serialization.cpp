// tests/test_Serialization.cpp

#include <chronograph/io/Serialization.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/repo/Repository.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <string>

using namespace chronograph;
namespace fs = std::filesystem;

namespace {

// Repo with a three-way merge on main, a side branch, and staged work on dev
Repository sampleRepository() {
    auto repo = Repository::init("main");
    repo.addNode("a", {{"name","A"}}, 1);
    repo.commit("add a");
    repo.branch("dev");
    repo.branch("feat");

    repo.checkout("feat");
    repo.addNode("f", {}, 2);
    repo.addEdge("af", "a", "f", {{"w","3"}}, 3);
    repo.commit("add f");

    repo.checkout("dev");
    repo.addNode("d", {}, 4);
    repo.commit("add d");

    repo.checkout("main");
    repo.updateNode("a", {{"name","A2"}}, 5);
    repo.commit("rename a");
    repo.merge("feat");

    repo.checkout("dev");
    repo.updateNode("d", {{"wip","yes"}}, 6);  // left uncommitted
    return repo;
}

}  // namespace

TEST(Serialization, GraphRoundTrip) {
    Graph g;
    g.addNode("x", {{"k","v"}}, 1);
    g.addNode("y", {}, 2);
    g.addEdge("e", "x", "y", {}, 3);
    g.updateEdge("e", {{"w","1"}}, 4);
    g.delNode("y", 5);

    const std::string json = io::graphToJson(g);
    Graph loaded = io::graphFromJson(json);

    EXPECT_EQ(loaded.getEventLog(), g.getEventLog());
    EXPECT_EQ(loaded.getNodes(), g.getNodes());
    EXPECT_EQ(loaded.getEdges(), g.getEdges());
    EXPECT_EQ(io::graphToJson(loaded), json);  // deterministic
}

TEST(Serialization, RepositoryRoundTrip) {
    Repository repo = sampleRepository();
    const std::string json = io::repositoryToJson(repo);
    Repository loaded = io::repositoryFromJson(json);

    EXPECT_EQ(io::repositoryToJson(loaded), json);  // deterministic
    EXPECT_EQ(loaded.currentBranch(), "dev");
    EXPECT_EQ(loaded.headCommit(), repo.headCommit());
    EXPECT_EQ(loaded.getCommitGraph().parents, repo.getCommitGraph().parents);
    for (const auto& branch : {"main", "dev", "feat"}) {
        EXPECT_EQ(loaded.graphAt(branch).getNodes(), repo.graphAt(branch).getNodes());
    }

    // Uncommitted work survives, and the loaded repo keeps working
    EXPECT_TRUE(loaded.hasUncommittedChanges());
    EXPECT_EQ(loaded.graph().getNodes(), repo.graph().getNodes());
    loaded.commit("wip");
    loaded.checkout("main");
    EXPECT_EQ(loaded.graph().getNodes().at("a").attributes.at("name"), "A2");
    EXPECT_EQ(loaded.graph().getEdges().at("af").createdTimestamp, 3);
}

TEST(Serialization, FileSaveAndLoad) {
    const fs::path dir = fs::temp_directory_path() / "chronograph_test_io";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const fs::path path = dir / "repo.json";

    Repository repo = sampleRepository();
    io::saveRepository(repo, path);
    repo.commit("more");
    io::saveRepository(repo, path);  // overwrite in place

    Repository loaded = io::loadRepository(path);
    EXPECT_FALSE(loaded.hasUncommittedChanges());
    EXPECT_EQ(loaded.headCommit(), repo.headCommit());
    EXPECT_FALSE(fs::exists(dir / "repo.json.tmp"));

    // File-system errors are not format errors
    try {
        io::loadGraph(dir / "missing.json");
        FAIL() << "expected an exception";
    } catch (const io::FormatError&) {
        FAIL() << "missing file reported as FormatError";
    } catch (const std::runtime_error&) {
    }
    fs::remove_all(dir);
}

TEST(Serialization, RejectsInvalidDocuments) {
    auto repoDoc = [](const std::string& body) {
        return R"({"format":"chronograph.repository","version":1,)" + body + "}";
    };
    const std::string root = R"({"id":"r","parents":[],"events":[]})";

    // Not JSON, wrong kind, unsupported version
    EXPECT_THROW(io::graphFromJson("{not json"), io::FormatError);
    EXPECT_THROW(io::repositoryFromJson(io::graphToJson(Graph{})), io::FormatError);
    EXPECT_THROW(io::graphFromJson(R"({"format":"chronograph.graph","version":99,"events":[]})"),
                 io::FormatError);
    // Missing field, unknown event type
    EXPECT_THROW(io::graphFromJson(R"({"format":"chronograph.graph","version":1})"),
                 io::FormatError);
    EXPECT_THROW(io::graphFromJson(R"({"format":"chronograph.graph","version":1,"events":[
                     {"id":"1","timestamp":1,"type":"EXPLODE","entity":"x"}]})"),
                 io::FormatError);
    // Inconsistent history
    EXPECT_THROW(io::repositoryFromJson(repoDoc(R"("head":"main","branches":{"main":"c"},
                     "commits":[)" + root + R"(,{"id":"c","parents":["nope"],"events":[]}]
                 )")), io::FormatError);
    EXPECT_THROW(io::repositoryFromJson(repoDoc(R"("head":"main","branches":{"main":"gone"},
                     "commits":[)" + root + "]")), io::FormatError);
    EXPECT_THROW(io::repositoryFromJson(repoDoc(R"("head":"other","branches":{"main":"r"},
                     "commits":[)" + root + "]")), io::FormatError);

    // Minimal valid document loads
    auto repo = io::repositoryFromJson(repoDoc(R"("head":"main","branches":{"main":"r"},
                    "commits":[)" + root + "]"));
    EXPECT_EQ(repo.headCommit(), "r");
}
