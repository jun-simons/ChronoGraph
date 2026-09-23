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
    Commit root{ rootId, {}, {}, "" };
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

bool Repository::hasUncommittedChanges() const {
    return workingGraph_.getEventLog().size() > lastCommittedEventIndex_;
}

void Repository::checkout(const std::string& branchName) {
    auto it = branches_.find(branchName);
    if (it == branches_.end()) {
        throw std::runtime_error("Branch '" + branchName + "' does not exist");
    }
    const std::string& target = it->second;

    // Same commit: just switch branch, carrying any uncommitted work along
    if (target == HEAD_commitId_) {
        HEAD_ = branchName;
        return;
    }
    if (hasUncommittedChanges()) {
        throw std::runtime_error(
            "Cannot checkout '" + branchName + "': uncommitted changes");
    }
    HEAD_ = branchName;
    moveHeadTo(target);
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

    if (hasUncommittedChanges()) {
        throw std::runtime_error(
            "Cannot merge '" + branchName + "': uncommitted changes");
    }

    // 3) Ancestors of our tip
    std::vector<std::string> ancA;
    std::unordered_set<std::string> setA;
    buildAncestors(A, ancA, setA);

    // 4) B already contained in A: nothing to do
    if (setA.count(B)) {
        return MergeResult{ A, {} };
    }

    // 5) A is an ancestor of B: fast forward
    std::vector<std::string> ancB;
    std::unordered_set<std::string> setB;
    buildAncestors(B, ancB, setB);
    if (setB.count(A)) {
        moveHeadTo(B);
        branches_[HEAD_] = B;
        return MergeResult{ B, {} };
    }

    // 6) Three-way merge
    // 6a) Base = most recent commit on B's first-parent chain that A also has.
    //     The root is shared by every chain, so this always exists.
    const auto chainB = firstParentChain(B);
    auto baseIt = std::find_if(chainB.rbegin(), chainB.rend(),
        [&](const std::string& cid) { return setA.count(cid) > 0; });
    if (baseIt == chainB.rend()) {
        throw std::runtime_error("No common ancestor found!");
    }

    // 6b) Replay B's commits after the base onto the working tree (which is at A)
    std::vector<Conflict> conflicts;
    std::vector<Event>   mergedEvents;
    for (auto it = chainB.begin() + (chainB.rend() - baseIt); it != chainB.end(); ++it) {
        for (auto& e : commits_.at(*it).events) {
            //TODO: detailed conflict population
            bool conflict = false;
            if (conflict && policy == MergePolicy::OURS) {
            // skip applying
            } else {
            // either no conflict, or THEIRS/UNION
                workingGraph_.addEvent(e);
                mergedEvents.push_back(e);
            }
        }
    }

    // 6c) Create the merge commit with two parents (A and B)
    std::string mergeId = generateCommitId();
    Commit m{ mergeId, { A, B }, std::move(mergedEvents),
              "Merge branch '" + branchName + "' into " + HEAD_ };
    commits_.emplace(mergeId, std::move(m));

    // advance HEAD on this branch
    branches_[HEAD_] = mergeId;
    HEAD_commitId_  = mergeId;
    lastCommittedEventIndex_ = workingGraph_.getEventLog().size();

    return MergeResult{ mergeId, std::move(conflicts) };
}

// ——— Inspecting history ———

const Commit& Repository::getCommit(const std::string& commitId) const {
    auto it = commits_.find(commitId);
    if (it == commits_.end()) {
        throw std::runtime_error("Commit '" + commitId + "' does not exist");
    }
    return it->second;
}

Graph Repository::graphAt(const std::string& ref) const {
    const auto chain = firstParentChain(resolve(ref));
    Graph g;
    replayCommits(g, chain.begin(), chain.end());
    return g;
}

DiffResult Repository::diff(const std::string& fromRef, const std::string& toRef) const {
    return chronograph::diff(graphAt(fromRef), graphAt(toRef));
}

// ——— Persistence support ———

RepositoryData Repository::exportData() const {
    RepositoryData data;
    data.branches.insert(branches_.begin(), branches_.end());
    data.head = HEAD_;

    const auto& log = workingGraph_.getEventLog();
    data.staged.assign(log.begin() + lastCommittedEventIndex_, log.end());

    // Topological order (parents first), deterministic for a given history:
    // iterative post-order DFS from each branch tip in name order, then any
    // commits no branch reaches, in ID order.
    std::vector<std::string> roots;
    for (const auto& [name, cid] : data.branches) roots.push_back(cid);
    std::vector<std::string> rest;
    for (const auto& [cid, _] : commits_) rest.push_back(cid);
    std::sort(rest.begin(), rest.end());
    roots.insert(roots.end(), rest.begin(), rest.end());

    std::unordered_set<std::string> visited;
    for (const auto& root : roots) {
        // stack of (commit, index of next parent to visit)
        std::vector<std::pair<std::string, size_t>> stack;
        if (visited.insert(root).second) stack.emplace_back(root, 0);
        while (!stack.empty()) {
            auto& [cid, next] = stack.back();
            const auto& parents = commits_.at(cid).parents;
            if (next < parents.size()) {
                const std::string& pid = parents[next++];
                if (visited.insert(pid).second) stack.emplace_back(pid, 0);
            } else {
                data.commits.push_back(commits_.at(cid));
                stack.pop_back();
            }
        }
    }
    return data;
}

Repository Repository::fromData(RepositoryData data) {
    auto invalid = [](const std::string& msg) {
        return std::invalid_argument("Invalid repository data: " + msg);
    };

    Repository repo;
    std::string rootId;
    for (auto& c : data.commits) {
        if (repo.commits_.count(c.id)) throw invalid("duplicate commit '" + c.id + "'");
        if (c.parents.empty()) {
            if (!rootId.empty()) throw invalid("more than one root commit");
            rootId = c.id;
        }
        for (const auto& pid : c.parents) {
            if (!repo.commits_.count(pid)) {
                throw invalid("commit '" + c.id + "' has unknown or later parent '" + pid + "'");
            }
        }
        std::string id = c.id;
        repo.commits_.emplace(std::move(id), std::move(c));
    }
    if (rootId.empty()) throw invalid("no root commit");

    for (const auto& [name, cid] : data.branches) {
        if (!repo.commits_.count(cid)) {
            throw invalid("branch '" + name + "' points at unknown commit '" + cid + "'");
        }
        repo.branches_[name] = cid;
    }
    auto head = repo.branches_.find(data.head);
    if (head == repo.branches_.end()) throw invalid("unknown head branch '" + data.head + "'");

    // Rebuild the working graph: committed history, then staged events
    repo.HEAD_ = data.head;
    repo.moveHeadTo(head->second);  // HEAD_commitId_ is empty: full rebuild
    for (const auto& e : data.staged) repo.workingGraph_.addEvent(e);
    return repo;
}

// ——— Helpers ———

std::string Repository::resolve(const std::string& ref) const {
    if (auto it = branches_.find(ref); it != branches_.end()) return it->second;
    if (commits_.count(ref)) return ref;
    throw std::runtime_error("Unknown branch or commit '" + ref + "'");
}

std::vector<std::string> Repository::firstParentChain(const std::string& cid) const {
    std::vector<std::string> chain;
    for (const Commit* c = &commits_.at(cid); ; c = &commits_.at(c->parents.front())) {
        chain.push_back(c->id);
        if (c->parents.empty()) break;
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

void Repository::moveHeadTo(const std::string& target) {
    const auto chain = firstParentChain(target);
    auto cur = std::find(chain.begin(), chain.end(), HEAD_commitId_);

    if (cur != chain.end()) {
        // Target descends from the current commit: replay only what's missing
        ++cur;
    } else {
        // Otherwise rebuild from the root
        workingGraph_.clearGraph();
        cur = chain.begin();
    }
    replayCommits(workingGraph_, cur, chain.end());

    HEAD_commitId_ = target;
    lastCommittedEventIndex_ = workingGraph_.getEventLog().size();
}

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