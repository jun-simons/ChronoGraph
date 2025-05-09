#include <chronograph/repo/utils/printUtils.h>
#include <chronograph/repo/Repository.h>
#include <iostream>

namespace chronograph {

void printCommitGraph(const CommitGraph& dag, std::ostream& out) {
    out << "Commit DAG:\n";
    for (const auto& cid : dag.commitIds) {
        out << " Commit " << cid << "\n";
        out << "   Parents:";
        if (auto pit = dag.parents.find(cid); pit != dag.parents.end() && !pit->second.empty()) {
            for (auto& p : pit->second) out << " " << p;
        } else {
            out << " (none)";
        }
        out << "\n   Children:";
        if (auto cit = dag.children.find(cid); cit != dag.children.end() && !cit->second.empty()) {
            for (auto& c : cit->second) out << " " << c;
        } else {
            out << " (none)";
        }
        out << "\n";
    }
}

} // namespace chronograph
