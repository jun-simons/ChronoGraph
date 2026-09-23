// include/chronograph/io/Serialization.h
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

namespace chronograph {

class Graph;
class Repository;

/// Saving and loading graphs and repositories as versioned JSON documents.
// * A Graph is stored as its event log; a Repository as its commits, branches,
//   HEAD and any uncommitted events. Live state is rebuilt on load.
// * Output is deterministic for a given history, so saved files diff cleanly
//   under version control.
namespace io {

/// Version written by this build; loading accepts versions up to this one.
inline constexpr int kFormatVersion = 1;

/// The input isn't a valid ChronoGraph document (malformed JSON, missing
/// fields, wrong document kind, unsupported version, inconsistent history).
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// ——— JSON text ———
std::string graphToJson(const Graph& graph);
Graph graphFromJson(const std::string& json);

std::string repositoryToJson(const Repository& repo);
Repository repositoryFromJson(const std::string& json);

// ——— Files ———
// Saves write to a temporary file and rename it into place, so an existing
// file is never left half-written. File-system failures throw
// std::runtime_error; bad contents throw FormatError.
void saveGraph(const Graph& graph, const std::filesystem::path& path);
Graph loadGraph(const std::filesystem::path& path);

void saveRepository(const Repository& repo, const std::filesystem::path& path);
Repository loadRepository(const std::filesystem::path& path);

}  // namespace io
}  // namespace chronograph
