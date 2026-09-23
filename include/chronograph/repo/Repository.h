#pragma once

#include <chronograph/graph/Graph.h>
#include <chronograph/graph/Diff.h>
#include <chronograph/graph/Event.h> 
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace chronograph {

// --- Commit Types ---
/// Represents single Commit in the repository
// * these form a DAG of commits in the Repository
// * events for every commit are stored in a vector
struct Commit {
    std::string id;         // commit hash/UUID
    std::vector<std::string> parents;  // parent commit IDs (1 or 2 for merges)
    std::vector<Event> events;    // the delta introduced by this commit
    std::string message;
};

// A simple representation of the commit DAG
// * ONLY used to return the commit DAG in an organized way
struct CommitGraph {
  // All commit IDs in the repo
  std::vector<std::string>                                commitIds;
  // For each commit ID, its list of parent commit IDs (1 or 2 elements)
  std::unordered_map<std::string, std::vector<std::string>> parents;
  // For each commit ID, its list of child commit IDs
  std::unordered_map<std::string, std::vector<std::string>> children;
};

// -- Merging and Conflict Types ---
enum class MergePolicy { OURS, THEIRS, ATTRIBUTE_UNION, INTERACTIVE };
struct Conflict {
  enum Kind { ADD_ADD, DEL_UPDATE, UPDATE_UPDATE } kind;
  Event ours, theirs;
};
struct MergeResult {
    std::string         mergeCommitId;
    std::vector<Conflict> conflicts;  // empty if no conflicts or non-interactive
  };

/// Everything needed to reconstruct a Repository (see exportData / fromData).
// * Plain data with no behaviour: persistence formats read and write this
//   rather than reaching into Repository's internals.
struct RepositoryData {
    std::vector<Commit> commits;                  // parents listed before children
    std::map<std::string, std::string> branches;  // branch name -> commit ID
    std::string head;                             // checked-out branch
    std::vector<Event> staged;                    // uncommitted events on top of head
};

// Git-style repository for a graph
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

    /// True if the working graph has events that haven't been committed yet
    bool hasUncommittedChanges() const;

    /// Switch HEAD to the tip of `branchName`, rebuilding the working graph.
    /// Throws std::runtime_error if the branch doesn't exist, or if there are
    /// uncommitted changes and the branch points at a different commit.
    void checkout(const std::string& branchName);

    /// List all branch names
    std::vector<std::string> listBranches() const;

    /// List all commits on the named branch (from root → tip)
    std::vector<Commit> listCommits(const std::string& branchName) const;
    
    /// Return a snapshot of the entire commit DAG: nodes + parent/child edges
    CommitGraph getCommitGraph() const;

    /**
     * Merge `branchName` into the current HEAD branch.
     * - policy: automatic resolution strategy
     * - in INTERACTIVE mode, conflicts are returned for manual resolution
     * Throws std::runtime_error if the branch doesn't exist or there are
     * uncommitted changes.
     */
    MergeResult merge(const std::string& branchName,
                    MergePolicy policy = MergePolicy::OURS);

    /// Access the current working‐tree graph
    const Graph& graph() const { return workingGraph_; }

    // ——— Inspecting history ———
    // A `ref` is a branch name or a commit ID (branch names take precedence).
    // All of these throw std::runtime_error for unknown refs.

    /// Name of the checked-out branch
    const std::string& currentBranch() const { return HEAD_; }
    /// ID of the commit HEAD points at
    const std::string& headCommit() const { return HEAD_commitId_; }

    /// Look up a single commit by ID
    const Commit& getCommit(const std::string& commitId) const;

    /// The committed graph as of `ref`, rebuilt from history. Its event log is
    /// that commit's full history, so Snapshots and temporal queries work on it.
    Graph graphAt(const std::string& ref) const;

    /// Changes between the committed graphs at two refs
    DiffResult diff(const std::string& fromRef, const std::string& toRef) const;

    // ——— Persistence support ———

    /// Copy out the full repository: commits, branches, HEAD and staged events
    RepositoryData exportData() const;

    /// Rebuild a repository from exported data. Throws std::invalid_argument if
    /// the data is inconsistent (unknown parents or branch targets, commits out
    /// of order, not exactly one root commit, or an unknown head branch).
    static Repository fromData(RepositoryData data);

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
    size_t lastCommittedEventIndex_ = 0;

    // Commits from the root to `cid`, following first parents. Each commit's
    // events are its delta against its first parent, so replaying this chain
    // reproduces the graph at `cid`.
    std::vector<std::string> firstParentChain(const std::string& cid) const;

    // Commit ID for a branch name or commit ID; throws if neither
    std::string resolve(const std::string& ref) const;

    // Append the events of commits [first, last) to `g`
    template <typename It>
    void replayCommits(Graph& g, It first, It last) const {
        for (; first != last; ++first) {
            for (const auto& e : commits_.at(*first).events) g.addEvent(e);
        }
    }

    // Point HEAD_commitId_ at `target` and bring the working graph in line with
    // it (replaying only the missing commits when possible)
    void moveHeadTo(const std::string& target);

    // helper to gather ancestors in topological order
    void buildAncestors(const std::string& cid,
                        std::vector<std::string>& out,
                        std::unordered_set<std::string>& seen) const;
};

}  // namespace chronograph
