#include <chronograph/repo/Repository.h>
#include <chronograph/graph/Snapshot.h>
#include <algorithm>
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
std::string Repository::commit(const std::string& message) {
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
    Commit c{ newId, { HEAD_commitId_ }, std::move(delta), message };
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

void Repository::checkout(const std::string& branchName) {
    auto it = branches_.find(branchName);
     if (it == branches_.end()) {
         throw std::runtime_error("Branch '" + branchName + "' does not exist");
     }
    const std::string newCommit = it->second;
    const std::string oldCommit = HEAD_commitId_;
    // 1) Switch HEAD to the target branch
    HEAD_ = branchName;
    HEAD_commitId_ = newCommit;

    // 2) If nothing changed, no need to update graph
    if (newCommit == oldCommit) {
        // make sure lastCommittedEventIndex_ stays correct
        lastCommittedEventIndex_ = workingGraph_.getEventLog().size();
        return;
    }

    // 3) Otherwise, rebuild or fast‐forward…
    // Collect oldChain from old HEAD, newChain from newCommit
    std::vector<std::string> oldChain, newChain;
    std::unordered_set<std::string> seen;
    buildAncestors(oldCommit, oldChain, seen);
    seen.clear();
    buildAncestors(newCommit, newChain, seen);

    bool isDescendant = false;
    if (oldChain.size() <= newChain.size()) {
      isDescendant = std::equal(
        oldChain.begin(), oldChain.end(),
        newChain.begin()
      );
    }

    // if old chain is a descendant of the new chain
    // * we fast forward new commits to match the branch
    // else if the old chain is a descendant of the new chain
    // * rebuild the chain based on prior commits
    if (isDescendant) { 
        // fast‐forward only the missing commits
        auto it2 = std::find(newChain.begin(), newChain.end(), oldCommit);
        ++it2;
        for (; it2 != newChain.end(); ++it2) {
            const Commit& cm = commits_.at(*it2);
            for (auto& e : cm.events) {
                workingGraph_.addEvent(e);
                workingGraph_.applyEvent(e);
            }
        }
    } else {
        // full rebuild
        workingGraph_.clearGraph();
        for (auto& cid : newChain) {
            const Commit& cm = commits_.at(cid);
            for (auto& e : cm.events) {
                workingGraph_.addEvent(e);
                workingGraph_.applyEvent(e);
            }
        }
    }

    // update how many events we have in the log now
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

CommitGraph Repository::getCommitGraph() const {
    CommitGraph g;

    // 1) Collect all commit IDs
    g.commitIds.reserve(commits_.size());
    for (const auto& [cid, cm] : commits_) {
        g.commitIds.push_back(cid);
    }

    // 2) Build parents map (copy directly from each Commit)
    for (const auto& [cid, cm] : commits_) {
        g.parents[cid] = cm.parents;
    }

    // 3) Build children map by inverting parents
    //    Initialize empty vectors for every commit
    for (const auto& cid : g.commitIds) {
        g.children[cid];  // make sure key exists
    }
    //    For each commit, register it as a child of each parent
    for (const auto& [cid, cm] : commits_) {
        for (const auto& pid : cm.parents) {
            g.children[pid].push_back(cid);
        }
    }

    return g;
}

MergeResult Repository::merge(const std::string& branchName,
    MergePolicy policy)
{
    // 1) Locate the branch tip
    auto bit = branches_.find(branchName);
    if (bit == branches_.end()) {
        throw std::runtime_error("Branch '" + branchName + "' does not exist");
    }
    const std::string A = HEAD_commitId_;
    const std::string B = bit->second;

    // 2) Trivial case: merging into itself
    if (A == B) {
        return MergeResult{ A, {} };
    }

    // 3) Build ancestor chains and sets
    std::vector<std::string> ancA, ancB;
    std::unordered_set<std::string> setA, setB;
    buildAncestors(A, ancA, setA);
    buildAncestors(B, ancB, setB);

    // 4) IF A is ancestor of B: simply fast forward
    if (setB.count(A)) {
        // compute linear path B->A (exclusive of A), using first‐parent pointers
        std::vector<std::string> pathB;
        for (std::string cid = B; cid != A; cid = commits_[cid].parents[0]) {
            pathB.push_back(cid);
        }
        std::reverse(pathB.begin(), pathB.end());

        // apply each commit’s events in order
        for (auto& cid : pathB) {
            for (auto& e : commits_.at(cid).events) {
                workingGraph_.addEvent(e);
                workingGraph_.applyEvent(e);
            }
        }

        // advance main branch pointer
        branches_[HEAD_] = B;
        HEAD_commitId_ = B;
        lastCommittedEventIndex_ = workingGraph_.getEventLog().size();
        return MergeResult{ B, {} };
    }

    // 5) IF True three‐way merge
    // 5a) Find the common ancestor (CA), walk back from B until we hit something in ancA
    std::string CA;
    for (auto it = ancB.rbegin(); it != ancB.rend(); ++it) {
        if (setA.count(*it)) {
            CA = *it;
            break;
        }
    }
    if (CA.empty()) {
        throw std::runtime_error("No common ancestor found!");
    }

    // 5b) Compute B’s delta since CA: linear path from CA→B
    std::vector<std::string> pathB;
    for (std::string cid = B; cid != CA; cid = commits_[cid].parents[0]) {
        pathB.push_back(cid);
    }
    std::reverse(pathB.begin(), pathB.end());

    // 5c) Apply B’s delta onto the current working‐tree (which is at A)
    std::vector<Conflict> conflicts;    
    std::vector<Event>   mergedEvents;
    for (auto& cid : pathB) {
        for (auto& e : commits_.at(cid).events) {
            //TODO: detailed conflict population
            bool conflict = false;
            if (conflict && policy == MergePolicy::OURS) {
            // skip applying
            } else {
            // either no conflict, or THEIRS/UNION
                workingGraph_.addEvent(e);
                workingGraph_.applyEvent(e);
                mergedEvents.push_back(e);
            }
        }
    }

    // 5d) Create the merge commit with two parents (A and B)
    std::string mergeId = generateCommitId();
    Commit m{ mergeId, { A, B }, std::move(mergedEvents) };
    commits_.emplace(mergeId, std::move(m));

    // advance HEAD on this branch
    branches_[HEAD_] = mergeId;
    HEAD_commitId_  = mergeId;
    lastCommittedEventIndex_ = workingGraph_.getEventLog().size();

    return MergeResult{ mergeId, std::move(conflicts) };
}

// ——— Helpers ———

void Repository::buildAncestors(const std::string& cid,
                                std::vector<std::string>& out,
                                std::unordered_set<std::string>& seen) const {
    if (!seen.insert(cid).second) return;
    const Commit& cm = commits_.at(cid);
    for (auto& pid : cm.parents) {
        buildAncestors(pid, out, seen);
    }
    out.push_back(cid);
}

}  // namespace chronograph