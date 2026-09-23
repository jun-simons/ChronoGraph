// src/io/Serialization.cpp
#include <chronograph/io/Serialization.h>
#include <chronograph/graph/Graph.h>
#include <chronograph/repo/Repository.h>

#include <nlohmann/json.hpp>

#include <array>
#include <fstream>
#include <sstream>
#include <utility>

namespace chronograph {
namespace io {

// Keys are written in insertion order so documents read top-down: header,
// then content, with each record's identifying fields first
using json = nlohmann::ordered_json;

namespace {

constexpr const char* kGraphKind      = "chronograph.graph";
constexpr const char* kRepositoryKind = "chronograph.repository";

// ——— Events ———

constexpr std::array<std::pair<EventType, const char*>, 6> kEventTypeNames{{
    {EventType::ADD_NODE,    "ADD_NODE"},
    {EventType::DEL_NODE,    "DEL_NODE"},
    {EventType::ADD_EDGE,    "ADD_EDGE"},
    {EventType::DEL_EDGE,    "DEL_EDGE"},
    {EventType::UPDATE_NODE, "UPDATE_NODE"},
    {EventType::UPDATE_EDGE, "UPDATE_EDGE"},
}};

const char* toString(EventType type) {
    for (const auto& [t, name] : kEventTypeNames) {
        if (t == type) return name;
    }
    throw FormatError("Unknown event type");
}

EventType eventTypeFromString(const std::string& s) {
    for (const auto& [t, name] : kEventTypeNames) {
        if (s == name) return t;
    }
    throw FormatError("Unknown event type '" + s + "'");
}

// Empty payload / endpoints are omitted to keep files small
json toJson(const Event& e) {
    json j = {
        {"id", e.id},
        {"type", toString(e.type)},
        {"entity", e.entityId},
    };
    if (!e.from.empty())    j["from"] = e.from;
    if (!e.to.empty())      j["to"] = e.to;
    j["timestamp"] = e.timestamp;
    if (!e.payload.empty()) j["payload"] = e.payload;
    return j;
}

Event eventFromJson(const json& j) {
    Event e;
    e.id        = j.at("id").get<std::string>();
    e.timestamp = j.at("timestamp").get<std::int64_t>();
    e.type      = eventTypeFromString(j.at("type").get<std::string>());
    e.entityId  = j.at("entity").get<std::string>();
    e.payload   = j.value("payload", std::map<std::string, std::string>{});
    e.from      = j.value("from", std::string{});
    e.to        = j.value("to", std::string{});
    return e;
}

json eventsToJson(const std::vector<Event>& events) {
    json arr = json::array();
    for (const auto& e : events) arr.push_back(toJson(e));
    return arr;
}

std::vector<Event> eventsFromJson(const json& arr) {
    std::vector<Event> events;
    for (const auto& j : arr) events.push_back(eventFromJson(j));
    return events;
}

// ——— Commits ———

json toJson(const Commit& c) {
    return {
        {"id", c.id},
        {"parents", c.parents},
        {"message", c.message},
        {"events", eventsToJson(c.events)},
    };
}

Commit commitFromJson(const json& j) {
    return Commit{
        j.at("id").get<std::string>(),
        j.at("parents").get<std::vector<std::string>>(),
        eventsFromJson(j.at("events")),
        j.value("message", std::string{}),
    };
}

// ——— Documents ———

json header(const char* kind) {
    return {{"format", kind}, {"version", kFormatVersion}};
}

// Parse text and check it is a document of `kind` this build can read.
// JSON library errors are translated to FormatError by the caller.
json parseDocument(const std::string& text, const char* kind) {
    json doc = json::parse(text);
    if (!doc.is_object() || doc.value("format", std::string{}) != kind) {
        throw FormatError(std::string("Not a ") + kind + " document");
    }
    const int version = doc.at("version").get<int>();
    if (version < 1 || version > kFormatVersion) {
        throw FormatError("Unsupported " + std::string(kind) + " version " +
                          std::to_string(version));
    }
    return doc;
}

// Run `fn`, reporting any malformed-input error uniformly as FormatError
template <typename Fn>
auto translatingErrors(Fn fn) -> decltype(fn()) {
    try {
        return fn();
    } catch (const json::exception& ex) {
        throw FormatError(std::string("Malformed document: ") + ex.what());
    } catch (const std::invalid_argument& ex) {
        throw FormatError(ex.what());
    }
}

// ——— Files ———

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open '" + path.string() + "' for reading");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeFileAtomically(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::path tmp = path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot open '" + tmp.string() + "' for writing");
        out << text;
        out.close();
        if (!out) throw std::runtime_error("Failed writing '" + tmp.string() + "'");
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        throw std::runtime_error("Cannot replace '" + path.string() + "': " + ec.message());
    }
}

}  // namespace

// ——— Graph ———

std::string graphToJson(const Graph& graph) {
    json doc = header(kGraphKind);
    doc["events"] = eventsToJson(graph.getEventLog());
    return doc.dump(2);
}

Graph graphFromJson(const std::string& text) {
    return translatingErrors([&] {
        json doc = parseDocument(text, kGraphKind);
        Graph g;
        for (const auto& e : eventsFromJson(doc.at("events"))) g.addEvent(e);
        return g;
    });
}

// ——— Repository ———

std::string repositoryToJson(const Repository& repo) {
    const RepositoryData data = repo.exportData();

    json commits = json::array();
    for (const auto& c : data.commits) commits.push_back(toJson(c));

    json doc = header(kRepositoryKind);
    doc["head"]     = data.head;
    doc["branches"] = data.branches;
    doc["commits"]  = std::move(commits);
    doc["staged"]   = eventsToJson(data.staged);
    return doc.dump(2);
}

Repository repositoryFromJson(const std::string& text) {
    return translatingErrors([&] {
        json doc = parseDocument(text, kRepositoryKind);

        RepositoryData data;
        data.head     = doc.at("head").get<std::string>();
        data.branches = doc.at("branches").get<std::map<std::string, std::string>>();
        for (const auto& c : doc.at("commits")) data.commits.push_back(commitFromJson(c));
        data.staged   = eventsFromJson(doc.value("staged", json::array()));
        return Repository::fromData(std::move(data));
    });
}

// ——— Files ———

void saveGraph(const Graph& graph, const std::filesystem::path& path) {
    writeFileAtomically(path, graphToJson(graph));
}

Graph loadGraph(const std::filesystem::path& path) {
    return graphFromJson(readFile(path));
}

void saveRepository(const Repository& repo, const std::filesystem::path& path) {
    writeFileAtomically(path, repositoryToJson(repo));
}

Repository loadRepository(const std::filesystem::path& path) {
    return repositoryFromJson(readFile(path));
}

}  // namespace io
}  // namespace chronograph
