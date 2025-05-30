// src/chronograph/cli/main.cpp
#include <CLI/CLI.hpp>
#include <iostream>
#include <sstream>
#include <map>

#include "chronograph/repo/Repository.h"
#include "chronograph/graph/Snapshot.h"

using namespace chronograph;

static std::map<std::string,std::string>
parseKeyVals(const std::vector<std::string>& kvs) {
    std::map<std::string,std::string> out;
    for (auto &s : kvs) {
        auto pos = s.find('=');
        if (pos == std::string::npos) continue;
        out[s.substr(0,pos)] = s.substr(pos+1);
    }
    return out;
}

int main(int argc, char** argv) {
    CLI::App app{"ChronoGraph CLI"};

    // global flags
    std::string repoPath = ".";
    app.add_option("-r,--repo", repoPath, "Path to chronograph repo");

    // init
    auto init = app.add_subcommand("init", "Initialize a repo");
    std::string rootBranch = "main";
    init->add_option("-b,--branch", rootBranch, "Root branch name");
    
    // status
    auto status = app.add_subcommand("status", "Show working-tree status");

    // add-node
    auto addn = app.add_subcommand("add-node", "Add a node");
    std::string nid; long ts;
    std::vector<std::string> nodeAttrs;
    addn->add_option("--id", nid)->required();
    addn->add_option("--ts", ts)->required();
    addn->add_option("--attr", nodeAttrs, "key=val pairs");

    // similarly define del-node, update-node, add-edge, del-edge, update-edge...
    // commit
    auto commit = app.add_subcommand("commit", "Commit staged events");
    std::string msg;
    commit->add_option("-m,--message", msg, "Commit message");

    // branch
    auto branch = app.add_subcommand("branch", "Create a branch");
    std::string bname;
    branch->add_option("--name", bname)->required();

    // checkout
    auto co = app.add_subcommand("checkout", "Switch branch");
    co->add_option("--name", bname)->required();

    // merge
    auto merge  = app.add_subcommand("merge", "Merge branch into HEAD");
    std::string into;
    merge->add_option("--into", into)->required();
    std::string policy = "OURS";
    merge->add_option("--policy", policy)->check(CLI::IsMember({"OURS","THEIRS","ATTRIBUTE_UNION"}));

    // diff
    auto diff = app.add_subcommand("diff", "Show diff between timestamps");
    long t1, t2;
    diff->add_option("--from", t1)->required();
    diff->add_option("--to",   t2)->required();

    CLI11_PARSE(app, argc, argv);

    // Repository instance
    Repository repo = Repository::init(rootBranch);
    // If not init command, assume existing:
    // TODO: load from disk

    if (*init) {
        std::cout << "Initialized repo at '" << repoPath
                  << "' with root branch '" << rootBranch << "'\n";
        return 0;
    }
    if (*status) {
        auto nodes = repo.graph().getNodes();
        std::cout << "Nodes: ";
        for (auto& [id,_]:nodes) std::cout << id << " ";
        std::cout<<"\n";
        // similarly edges...
        return 0;
    }
    if (*addn) {
        auto attrs = parseKeyVals(nodeAttrs);
        repo.addNode(nid, attrs, ts);
        std::cout << "Staged add-node " << nid << "\n";
        return 0;
    }
    if (*commit) {
        auto cid = repo.commit(msg);
        std::cout << "Committed: " << cid << "\n";
        return 0;
    }
    if (*branch) {
        repo.branch(bname);
        std::cout << "Created branch: " << bname << "\n";
        return 0;
    }
    if (*co) {
        repo.checkout(bname);
        std::cout << "Checked out: " << bname << "\n";
        return 0;
    }
    if (*diff) {
        auto d = repo.graph().diff(t1,t2);
        std::cout << "Nodes added: ";
        for (auto& n: d.nodesAdded) std::cout<<n.id<<" ";
        std::cout<<"\n";
        return 0;
    }
    if (*merge) {
        MergePolicy pol = MergePolicy::OURS;
        if      (policy=="THEIRS")        pol = MergePolicy::THEIRS;
        else if (policy=="ATTRIBUTE_UNION") pol = MergePolicy::ATTRIBUTE_UNION;
        auto res = repo.merge(into, pol);
        std::cout << "Merged -> " << res.mergeCommitId << "\n";
        return 0;
    }

    // fallback: show help
    std::cout << app.help() << "\n";
    return 1;
}
