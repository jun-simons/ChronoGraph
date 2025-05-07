#include "chronograph/Repository.h"
#include <random>
#include <sstream>
#include <unordered_set>
#include <stdexcept>

namespace chronograph {
namespace {
/// Quick helper for random commit IDs;
// TODO: replace with better UUID generator
std::string generateCommitId() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    ss << std::hex << dist(rng);
    return ss.str();
}
}  // anon

Repository Repository::init(const std::string& rootBranch) {
    Repository repo;
    // Create an initial “root” commit
    const std::string rootId = generateCommitId();
    Commit root{ rootId, {}, {} };
    repo.commits_.emplace(rootId, root);

    // Set up branches and HEAD
    repo.branches_[rootBranch]     = rootId;
    repo.HEAD_                     = rootBranch;
    repo.HEAD_commitId_            = rootId;
    // No events committed yet
    repo.lastCommittedEventIndex_  = 0;

    return repo;
}

// --- MUTATORS for workign Graph ---- 
// * simply forward to graph functions
void Repository::addNode(const std::string& id,
    const std::map<std::string, std::string>& attrs,
    std::int64_t timestamp)
{
workingGraph_.addNode(id, attrs, timestamp);
}

void Repository::delNode(const std::string& id, std::int64_t timestamp) {
workingGraph_.delNode(id, timestamp);
}

void Repository::addEdge(const std::string& id,
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& attrs,
    std::int64_t timestamp)
{
workingGraph_.addEdge(id, from, to, attrs, timestamp);
}

void Repository::delEdge(const std::string& id, std::int64_t timestamp) {
workingGraph_.delEdge(id, timestamp);
}

void Repository::updateNode(const std::string& id,
       const std::map<std::string, std::string>& attrs,
       std::int64_t timestamp)
{
workingGraph_.updateNode(id, attrs, timestamp);
}

void Repository::updateEdge(const std::string& id,
       const std::map<std::string, std::string>& attrs,
       std::int64_t timestamp)
{
workingGraph_.updateEdge(id, attrs, timestamp);
}


// --- Commit staged events ---
std::string Repository::commit(const std::string&) {
    const auto& log = workingGraph_.getEventLog();
    size_t total = log.size();

    // Nothing new to commit?
    if (total <= lastCommittedEventIndex_) {
        return HEAD_commitId_;
    }

    // Slice out the new delta
    std::vector<Event> delta(
        log.begin() + lastCommittedEventIndex_,
        log.end()
    );

    // Make a commit (add to commits map)
    std::string newId = generateCommitId();
    Commit c{ newId, { HEAD_commitId_ }, std::move(delta) };
    commits_.emplace(newId, c);

    // Advance branch & HEAD
    branches_[HEAD_] = newId;
    HEAD_commitId_ = newId;
    lastCommittedEventIndex_ = total;

    return newId;
}

// ——— Branching ———
void Repository::branch(const std::string& branchName) {
    // point new branch at current HEAD commit
    branches_[branchName] = HEAD_commitId_;
}

// ——— Checkout ———
void Repository::checkout(const std::string& branchName) {
    auto it = branches_.find(branchName);
    if (it == branches_.end()) {
        throw std::runtime_error("Branch '" + branchName + "' does not exist");
    }
    HEAD_ = branchName;
    HEAD_commitId_ = it->second;

    // Rebuild the workingGraph_ from scratch
    // Collect all ancestor commit IDs in order
    std::vector<std::string> chain;
    std::unordered_set<std::string> seen;
    buildAncestors(HEAD_commitId_, chain, seen);

    // Wipe graph history & state
    workingGraph_.clearGraph();

    // Replay every commit’s events
    for (auto& cid : chain) {
        const Commit& cm = commits_.at(cid);
        for (auto& e : cm.events) {
            workingGraph_.addEvent(e);
            workingGraph_.applyEvent(e);
        }
    }

    // Log now has exactly chain’s events
    lastCommittedEventIndex_ = workingGraph_.getEventLog().size();
}

// ——— List branches & commits ———

std::vector<std::string> Repository::listBranches() const {
    std::vector<std::string> names;
    names.reserve(branches_.size());
    for (auto& [name,_] : branches_) {
        names.push_back(name);
    }
    return names;
}

std::vector<Commit> Repository::listCommits(const std::string& branchName) const {
    auto it = branches_.find(branchName);
    if (it == branches_.end()) {
        throw std::runtime_error("Branch '" + branchName + "' does not exist");
    }
    std::vector<std::string> chainIds;
    std::unordered_set<std::string> seen;
    buildAncestors(it->second, chainIds, seen);

    std::vector<Commit> chain;
    chain.reserve(chainIds.size());
    for (auto& cid : chainIds) {
        chain.push_back(commits_.at(cid));
    }
    return chain;
}

// ——— Helpers ———

void Repository::buildAncestors(const std::string& cid,
                                std::vector<std::string>& out,
                                std::unordered_set<std::string>& seen) const
{
    if (!seen.insert(cid).second) return;
    const Commit& cm = commits_.at(cid);
    for (auto& pid : cm.parents) {
        buildAncestors(pid, out, seen);
    }
    out.push_back(cid);
}

}  // namespace chronograph