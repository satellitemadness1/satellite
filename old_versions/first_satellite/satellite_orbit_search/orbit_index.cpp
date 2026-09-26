// Phase 4 — index. "does it already know the result? ... What do we know about
// this user/this machine/the code it runs." (orbit.txt)
//
// A file at $HOME/.satl_orbit, one record per line, and §9's terms for
// $HOME/.satl_history are the terms here: the file is REAL, it is READABLE, and
// there is a way to see it and clear it from inside the language, because
// keeping a record of what a user did is "the kind of thing this language does
// not do quietly". A hidden index would be the language acting on the user
// behind their back, which is the one thing it has never done.
//
// THE THIRD TERM IS NOT MET. No satellite program can see or clear this file --
// nothing under src/ reaches orbit_memory_read or orbit_memory_forget at all --
// so today this phase IS the quiet record its own paragraph refuses. The whole
// of it is in orbit_memory.hpp and in orbit_plan.txt's DECISION 9, and it is
// the first thing to fix in this module.
//
// THE RULE THAT KEEPS MEMORY HONEST: a remembered answer is offered only if it
// is PRESENT IN THE CURRENT CORPUS. Memory proposes; it never invents. Without
// that rule a stale record would put a value into a result that the searched
// structure does not contain -- and a result whose `value` is not in the thing
// searched is a lie whatever confidence is printed beside it.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

namespace satellite {
namespace {

// Where the file is, when a test or a program has not said otherwise.
// THREAD-LOCAL on DECISION 5a's argument for the dial: a knob meaning "where
// should MY memory live" is per-thread by construction, and it costs no lock in
// something that runs inside a search.
thread_local std::string g_memory_path;

// A field, made safe to put on one line. Tab separates and newline terminates,
// so both have to survive being inside a rendered value -- and a value can
// contain either, since a satellite string may hold any byte.
std::string escape(const std::string &text)
{
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '\t': out += "\\t";  break;
        case '\n': out += "\\n";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

std::string unescape(const std::string &text)
{
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != '\\' || i + 1 >= text.size()) {
            out += text[i];
            continue;
        }
        switch (text[++i]) {
        case 't': out += '\t'; break;
        case 'n': out += '\n'; break;
        default:  out += text[i]; break;
        }
    }
    return out;
}

void write_all(const std::vector<OrbitRecord> &records)
{
    const std::string path = orbit_memory_path();
    if (path.empty())
        return;

    std::ofstream file(path, std::ios::trunc);
    if (!file)
        return;
    for (const OrbitRecord &record : records)
        file << record.count << '\t' << escape(record.pattern) << '\t'
             << escape(record.resolved) << '\n';
}

} // namespace

std::string orbit_memory_path()
{
    if (!g_memory_path.empty())
        return g_memory_path;

    // A machine with no $HOME gets memory-only and nothing written, which is
    // the same answer §9 gives the history file rather than dropping a dotfile
    // in whatever directory the program happened to start in.
    const char *home = std::getenv("HOME");
    if (!home || !*home)
        return std::string();
    return std::string(home) + "/.satl_orbit";
}

void set_orbit_memory_path(const std::string &path) { g_memory_path = path; }

void orbit_memory_read(std::vector<OrbitRecord> &out)
{
    const std::string path = orbit_memory_path();
    if (path.empty())
        return;

    // A MISSING MEMORY IS A MEMORY WITH NOTHING IN IT, not an error. The first
    // search a user ever runs finds no file, and a phase that failed on that
    // would make the feature broken until it had been used once.
    std::ifstream file(path);
    if (!file)
        return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        const size_t one = line.find('\t');
        if (one == std::string::npos)
            continue;
        const size_t two = line.find('\t', one + 1);
        if (two == std::string::npos)
            continue;

        OrbitRecord record;
        record.count = std::strtoll(line.substr(0, one).c_str(), nullptr, 10);
        record.pattern = unescape(line.substr(one + 1, two - one - 1));
        record.resolved = unescape(line.substr(two + 1));
        if (record.count > 0)
            out.push_back(std::move(record));
    }
}

void orbit_memory_record(const std::string &pattern, const std::string &resolved)
{
    if (orbit_memory_path().empty())
        return;

    std::vector<OrbitRecord> records;
    orbit_memory_read(records);

    for (OrbitRecord &record : records)
        if (record.pattern == pattern && record.resolved == resolved) {
            record.count++;
            write_all(records);
            return;
        }

    records.push_back(OrbitRecord{pattern, resolved, 1});
    write_all(records);
}

void orbit_memory_forget()
{
    const std::string path = orbit_memory_path();
    if (!path.empty())
        std::remove(path.c_str());
}

void orbit_index(const OrbitQuery &query, std::vector<Finding> &out)
{
    std::vector<OrbitRecord> records;
    orbit_memory_read(records);
    if (records.empty())
        return;

    const std::string asked = to_string(*query.pattern);

    std::set<std::string> known;
    for (const Finding &finding : out)
        known.insert(finding_key(finding));

    // What the corpus actually holds, so a remembered answer can be checked
    // against it. This is the "memory proposes, never invents" rule in code:
    // the ValuePtr handed back below comes from THIS structure, not from the
    // file -- the file only says which of the corpus's own values to look at.
    std::vector<OrbitTerm> corpus;
    orbit_alphabet(query.root, query.max_depth, corpus);

    for (const OrbitRecord &record : records) {
        if (record.pattern != asked)
            continue;

        // --- confirming what the earlier phases already found ---------------
        //
        // FIRST, AND IT IS THE COMMON CASE. Memory's most useful sentence is
        // not "here is something new", it is "the thing you just found is the
        // thing this pattern has meant every other time" -- so the prior goes
        // onto the finding that is already there and phase 5 can rank "found
        // AND remembered" above "found".
        //
        // It is also the ONLY way a remembered RUN can ever be confirmed. A run
        // is a window over a list, not a node in it, so it does not appear in
        // the corpus alphabet below and the term loop can never match it. Built
        // without this, the memory recorded [true, true] on every run of
        // orbit.txt's own example and could not find it again once -- **the
        // count reached 22 in the file with `prior` still 0 in every result.**
        bool confirmed = false;
        for (Finding &existing : out)
            if (existing.value && !existing.constructed &&
                to_string(*existing.value) == record.resolved) {
                existing.prior = static_cast<int>(record.count);
                confirmed = true;
            }
        if (confirmed)
            continue;

        // --- proposing something the earlier phases missed -------------------
        for (const OrbitTerm &term : corpus) {
            if (!term.value || to_string(*term.value) != record.resolved)
                continue;

            // Where it is NOW. The record remembers what a pattern meant, never
            // where it was: a path is a fact about the structure in front of us
            // and the structure may have changed since. Re-finding it through
            // the search power is one walk and it cannot go stale.
            std::vector<SearchHit> hits;
            search_walk(query.root, term.value, SEARCH_EXACT, query.max_depth,
                        hits);

            for (const SearchHit &hit : hits) {
                Finding remembered(
                    hit.value, hit.key, hit.path, hit.score, ORBIT_INDEX,
                    "memory: " + asked + " has resolved to " + record.resolved +
                        " " + std::to_string(record.count) +
                        (record.count == 1 ? " time before" : " times before"));
                remembered.prior = static_cast<int>(record.count);

                const std::string id = finding_key(remembered);
                if (known.count(id)) {
                    // ALREADY FOUND BY AN EARLIER PHASE, and that is the most
                    // useful thing memory can say. It is not a duplicate to
                    // drop: the prior belongs on the finding that is already
                    // there, so phase 5 can weigh "we found it AND we have seen
                    // it before" above "we found it".
                    for (Finding &existing : out)
                        if (finding_key(existing) == id)
                            existing.prior = static_cast<int>(record.count);
                    continue;
                }
                known.insert(id);
                out.push_back(std::move(remembered));
            }
        }
    }
}

} // namespace satellite
