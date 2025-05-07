#pragma once

#include "Graph.h"
#include "Event.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace chronograph {

/// Represents single Commit in the repository
// * these form a DAG of commits in the Repository
// * events for every commit are stored in a vector
struct Commit {
    std::string id;         // commit hash/UUID
    std::vector<std::string> parents;  // parent commit IDs (1 or 2 for merges)
    std::vector<Event> events;    // the delta introduced by this commit
};

/// Simple git-style repository for a graph
class Repository {
public:
    /// Initialize an empty repo with a single root commit on `rootBranch`
    static Repository init(const std::string& rootBranch = "main");

    // ——— Working-tree mutators (for staging) ———
    void addNode(const std::string& id,
                 const std::map<std::string, std::string>& attrs,
                 std::int64_t timestamp);
    void delNode(const std::string& id, std::int64_t timestamp);
    void addEdge(const std::string& id,
                 const std::string& from,
                 const std::string& to,
                 const std::map<std::string, std::string>& attrs,
                 std::int64_t timestamp);
    void delEdge(const std::string& id, std::int64_t timestamp);
    void updateNode(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp);
    void updateEdge(const std::string& id,
                    const std::map<std::string, std::string>& attrs,
                    std::int64_t timestamp);

    /// Commit all staged events since the last commit. Returns the new commit ID.
    std::string commit(const std::string& message = "");

    /// Create a new branch at the current HEAD commit
    void branch(const std::string& branchName);

    /// Switch HEAD to the tip of `branchName`, rebuilding the working graph
    void checkout(const std::string& branchName);

    /// List all branch names
    std::vector<std::string> listBranches() const;

    /// List all commits on the named branch (from root → tip)
    std::vector<Commit> listCommits(const std::string& branchName) const;

    /// Access the current working‐tree graph
    const Graph& graph() const { return workingGraph_; }

private:
    Graph workingGraph_;

    // commit storage: commitId -> Commit
    std::unordered_map<std::string, Commit> commits_;
    // branch pointers: branchName -> commitId
    std::unordered_map<std::string, std::string> branches_;

    // current HEAD
    std::string HEAD_;           
    std::string HEAD_commitId_;  

    // how many events have been committed into parents already
    size_t lastCommittedEventIndex_;

    // helper to gather ancestors in topological order
    void buildAncestors(const std::string& cid,
                        std::vector<std::string>& out,
                        std::unordered_set<std::string>& seen) const;
};

}  // namespace chronograph
