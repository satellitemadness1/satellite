// satellite/bytecode/capsule_reach.cpp -- WHAT A WRITTEN NAME REACHES: the lookups
// over the scopes capsule_scopes.cpp found. The header says whose each rule is; the
// one rule still the author's to decide lives in CapsuleTable::reach, and its comment
// says where it widens.

#include "capsule_scopes.hpp"

#include "program_walk.hpp"
#include "../machine/source_position.hpp"

#include <algorithm>
#include <utility>

namespace satellite004 {

using token::Code;

std::vector<std::pair<std::string, std::size_t>> files_included_by(const CapsuleTable &table, std::size_t row)
{
    std::vector<std::pair<std::string, std::size_t>> files;
    if (row >= table.included.size())
        return files;
    for (const auto &[stem, scopes] : table.included[row])
        for (const std::size_t file : scopes)
            files.emplace_back(stem, file);
    std::sort(files.begin(), files.end(),
              [&table](const auto &a, const auto &b) { return table.scopes[a.second].row < table.scopes[b.second].row; });
    return files;
}

namespace {

// HOW A SENTENCE NAMES WHERE A WALK HAS GOT TO: a file, two files of one name, or a space.
std::string describe(const CapsuleTable &table, const std::vector<std::size_t> &at, const std::string &walked)
{
    const CapsuleScope &first = table.scopes[at.front()];
    if (first.parent != kNoScope)
        return "the satellite.namespace " + walked;
    if (at.size() == 1)
        return "the file " + first.file;
    std::string files = "the files " + first.file;
    for (std::size_t i = 1; i < at.size(); ++i)
        files += (i + 1 == at.size() ? " and " : ", ") + table.scopes[at[i]].file;
    return files;
}

} // namespace

std::size_t CapsuleTable::scope_at(std::size_t row, std::size_t at) const
{
    if (row >= in_row.size())
        return kNoScope;
    // IN THE ORDER THEY OPENED, so the LAST one holding `at` is the innermost: two
    // scopes of one row either nest or do not touch.
    std::size_t found = kNoScope;
    for (const std::size_t each : in_row[row])
        if (scopes[each].begins <= at && at < scopes[each].ends)
            found = each;
    return found;
}

const CapsuleSite *CapsuleTable::bare(std::size_t scope, const std::string &name) const
{
    for (std::size_t at = scope; at != kNoScope; at = scopes[at].parent) {
        const std::unordered_map<std::string, std::size_t>::const_iterator found = scopes[at].capsules.find(name);
        if (found != scopes[at].capsules.end())
            return &sites[found->second];
        // A SPACESUIT'S SUPERTYPES' CAPSULES ARE ITS OWN TOO (2026-09-22): the author's
        // eclipse, which extends view_forge, calls view_forge's `find_link_name(...)` by
        // its bare name. Its own come first, then each supertype's, nearest first.
        if (scopes[at].is_a_suit() && scopes[at].layout != nullptr)
            for (std::size_t n = 1; n < scopes[at].layout->lineage.size(); ++n) {
                const CapsuleScope &super = scopes[scopes[at].layout->lineage[n]];
                const std::unordered_map<std::string, std::size_t>::const_iterator inherited =
                    super.capsules.find(name);
                if (inherited != super.capsules.end())
                    return &sites[inherited->second];
            }
    }
    return nullptr;
}

// A HABIT FROM ANOTHER LANGUAGE: print("hello") is refused as "no capsule named print",
// which is true and tells a person meeting satellite nothing (the error sweep,
// 2026-09-25). The names people type first, and what satellite spells them.
namespace {
std::string what_satellite_calls(const std::string &name)
{
    for (const char *showing : {"print", "println", "printf", "puts", "echo", "write", "writeln", "say", "cout", "log"})
        if (name == showing)
            return " -- to show something, write satellite.console.display(...)";
    return std::string();
}
} // namespace

Reached CapsuleTable::reach(std::size_t scope, const std::vector<std::string> &names) const
{
    Reached answer;
    answer.code = satl_line_not_understood;
    if (scope == kNoScope || names.empty()) {
        answer.why = "no capsule named " + (names.empty() ? std::string() : names.front());
        return answer;
    }
    const std::size_t row = scopes[scope].row;

    if (names.size() == 1) {
        answer.site = bare(scope, names.front());
        if (answer.site != nullptr)
            return answer;
        // NOT DRAGGED INTO THE GLOBAL NAMESPACE (the author, 2026-09-13) -- but the
        // person is told the spelling that does reach it, which is all that is missing.
        for (const auto &[stem, file] : files_included_by(*this, row))
            if (scopes[file].capsules.count(names.front()) != 0) {
                answer.why = names.front() + " is a capsule of " + scopes[file].file + ", and a file's capsules are "
                             "reached through its name -- write " + stem + "." + names.front() + "(...)";
                return answer;
            }
        answer.why = "no capsule named " + names.front() + what_satellite_calls(names.front());
        return answer;
    }

    // THE FIRST NAME: a space around this scope, innermost first, and then a file this
    // file includes. A space is asked first so that a file's own name for something
    // is never hidden by a file it happens to include -- and join_includes refuses a
    // file whose own name is taken, so at the top of a file the order cannot matter.
    std::vector<std::size_t> at;
    for (std::size_t outward = scope; outward != kNoScope && at.empty(); outward = scopes[outward].parent) {
        const std::unordered_map<std::string, std::size_t>::const_iterator space = scopes[outward].spaces.find(names.front());
        if (space != scopes[outward].spaces.end())
            at.push_back(space->second);
    }
    if (at.empty()) {
        const auto file = included[row].find(names.front());
        if (file != included[row].end())
            at = file->second;
    }

    // A FILE REACHES ONLY THE FILES IT INCLUDES ITSELF -- AND THIS IS THE ONE PLACE
    // THAT RULE LIVES. It is 003's, and the author is still thinking about it
    // (2026-09-22: "I think differently for only the last thing"). Widening it is
    // here: give `at` the files that the included files include, instead of saying
    // below that they are not reached. Nothing else in satl assumes the narrow rule.
    if (at.empty()) {
        for (const auto &[stem, file] : files_included_by(*this, row)) {
            if (included[scopes[file].row].count(names.front()) == 0)
                continue;
            answer.through_a_scope = true;
            answer.why = names.front() + " is a file that " + scopes[file].file + " includes, and a file reaches "
                         "only the files it includes itself -- add satellite.include(" + names.front() +
                         ") to this file";
            return answer;
        }
        answer.why = "no capsule named " + names.back();
        return answer;           // not a space and not a file: the first name may be a variable
    }
    answer.through_a_scope = true;

    std::string walked = names.front();
    for (std::size_t n = 1; n + 1 < names.size(); ++n) {
        std::vector<std::size_t> deeper;
        for (const std::size_t each : at) {
            const std::unordered_map<std::string, std::size_t>::const_iterator space = scopes[each].spaces.find(names[n]);
            if (space != scopes[each].spaces.end())
                deeper.push_back(space->second);
        }
        if (deeper.empty()) {
            answer.why = describe(*this, at, walked) + " has no satellite.namespace named " + names[n];
            return answer;
        }
        at = std::move(deeper);
        walked += "." + names[n];
    }

    // THE CAPSULE, IN EXACTLY ONE OF THE PLACES THE WALK REACHED. Two is two files of
    // one name that BOTH declare it -- the one case the program could mean either.
    const std::string &last = names.back();
    std::vector<std::size_t> holding;
    for (const std::size_t each : at)
        if (scopes[each].capsules.count(last) != 0)
            holding.push_back(each);
    if (holding.size() == 1) {
        answer.site = &sites[scopes[holding.front()].capsules.at(last)];
        return answer;
    }
    if (holding.empty()) {
        bool a_space = false;
        for (const std::size_t each : at) a_space = a_space || scopes[each].spaces.count(last) != 0;
        answer.why = a_space ? walked + "." + last + " is a satellite.namespace and not a capsule, so it cannot be called"
                             : describe(*this, at, walked) + " declares no capsule named " + last;
        return answer;
    }
    answer.why = walked + "." + last + " could be either of two files this file includes -- " +
                 describe(*this, holding, walked) + " are both named " + names.front() + " and both declare " + last +
                 ", so rename one of the files";
    return answer;
}

std::string CapsuleTable::already_names(std::size_t scope, const std::string &name) const
{
    if (scope == kNoScope)
        return std::string();
    // A CAPSULE AND A SPACESUIT ARE NAMES TOO (M5, DESIGN §7: "a new name is checked
    // against all of them"). A variable called `helper` beside a capsule `helper()`, or
    // `thing` beside a spacesuit `thing`, is one name for two things -- the collision the
    // author ruled an error for spacesuits (POLYMORPH D9.3) -- and 003 let a variable hide
    // the capsule outright: its `helper()` then found nothing to call.
    for (std::size_t at = scope; at != kNoScope; at = scopes[at].parent) {
        const std::unordered_map<std::string, std::size_t>::const_iterator space = scopes[at].spaces.find(name);
        if (space != scopes[at].spaces.end())
            return "the satellite.namespace " + scopes[space->second].within;
        const std::string in = scopes[at].parent == kNoScope ? std::string(" of this file")
                               : scopes[at].is_a_suit()      ? " of the spacesuit " + scopes[at].within
                                                             : " of the satellite.namespace " + scopes[at].within;
        if (scopes[at].capsules.count(name) != 0)
            return "a capsule" + in;
        const std::unordered_map<std::string, std::size_t>::const_iterator suit = scopes[at].suits.find(name);
        if (suit != scopes[at].suits.end())
            return "the spacesuit " + scopes[suit->second].within;
    }
    if (included[scopes[scope].row].count(name) != 0)
        return "the file " + name + ".satl this file includes";
    return std::string();
}

const CapsuleSite *CapsuleTable::main() const
{
    if (file_scope.empty())
        return nullptr;
    return bare(file_scope.front(), "satellite.main");
}

const CapsuleSite *CapsuleTable::by_key(const std::string &key) const
{
    const std::unordered_map<std::string, std::size_t>::const_iterator found = keys.find(key);
    return found == keys.end() ? nullptr : &sites[found->second];
}

bool dotted_names_at(const std::vector<std::bitset<16>> &row, std::size_t &at, std::vector<std::string> &names)
{
    if (code_at(row, at) != token::name_token)
        return false;
    names.clear();
    std::size_t k = at;
    names.push_back(text_at(row, k));
    while (code_at(row, k) == token::method_token) {
        const Code next = code_at(row, k + 1);
        if (next == token::name_token) {
            ++k;
            names.push_back(text_at(row, k));
        } else if (token::is_method_code(next)) {
            // A METHOD'S NAME, READ BACK AS ITS FIRST SPELLING. The lexer keeps the
            // code and not the text, so a capsule named with a method's SECOND spelling
            // (`to_number`, whose first is `number`) reads back as the first -- and is
            // then refused as a capsule nobody declared, never run as another.
            names.push_back(token::method_name_of(next));
            k += 2;
        } else {
            break;
        }
    }
    at = k;
    return true;
}

std::string capsule_key_at(const CapsuleTable *table, const BytecodeRegistry *program,
                           const std::vector<std::bitset<16>> &row, std::size_t at,
                           const std::vector<std::string> &names)
{
    std::string written = names.empty() ? std::string() : names.front();
    for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];
    if (table == nullptr || program == nullptr)
        return written;
    const std::size_t which = row_index_of(program, row);
    const Reached reached = table->reach(table->scope_at(which, at), names);
    return reached.site != nullptr ? reached.site->key : written;
}

} // namespace satellite004
