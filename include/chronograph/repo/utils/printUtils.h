#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>

namespace chronograph {
struct CommitGraph;

/// Pretty-print the repository’s commit DAG.
void printCommitGraph(const CommitGraph& dag, std::ostream& out);
}
