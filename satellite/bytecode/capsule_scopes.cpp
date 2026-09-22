// satellite/bytecode/capsule_scopes.cpp -- FINDING THE SCOPES. The header says what a
// scope is and whose each rule is. Two pieces here: the scan that finds every scope
// and capsule of a file, and the pass that joins each include to the row it loaded.
// capsule_reach.cpp is the other half: what a written name reaches.

#include "capsule_scopes.hpp"

#include "include_shape.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"

#include <utility>

namespace satellite004 {
namespace {

using token::Code;

const Code kCapsule = word::code_of(1, 2);       // satellite.capsule
const Code kSpacesuit = word::code_of(1, 10);    // satellite.spacesuit
const Code kInclude = word::code_of(1, 1);       // satellite.include
const Code kNamespace = word::code_of(1, 28);    // satellite.namespace, and satellite.space

// HOW A SENTENCE NAMES A SCOPE: "this file", or the space by its dotted name.
std::string where_is(const CapsuleScope &scope)
{
    return scope.parent == kNoScope ? std::string("this file") : "the satellite.namespace " + scope.within;
}

void refuse(CapsuleTable &table, std::size_t row, std::size_t at, signed long long int code, std::string why)
{
    table.troubles.push_back(ScopeTrouble{row, at, code, std::move(why)});
}

// A NAME IS DECLARED ONCE IN A SCOPE (the author, POLYMORPH D9.3: "a class declared
// twice is an ERROR: name collision"). True when `name` is free in `scope`; otherwise
// the refusal is recorded against the second declaration, which is the one to change.
bool free_in(CapsuleTable &table, std::size_t scope, const std::string &name, std::size_t row, std::size_t at,
             const char *declaring)
{
    const CapsuleScope &in = table.scopes[scope];
    const char *already = in.capsules.count(name) != 0 ? "a capsule"
                          : in.spaces.count(name) != 0 ? "a satellite.namespace" : nullptr;
    if (already == nullptr)
        return true;
    refuse(table, row, at, name_declared_twice,
           name + " is declared twice in " + where_is(in) + " -- it is already " + already + " there, and this " +
               declaring + " would be a second thing with the same name");
    return false;
}

// ONE CAPSULE'S HEADER, from the satellite.capsule code at `at`: its name, its
// parameters, and the `{` its body opens with. Answers the brace's position, or 0
// when this is not a declaration after all -- the same rule the scan had before
// scopes, and the same trouble sentences, which check_program now points a caret at.
std::size_t capsule_header(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &name,
                           std::vector<CapsuleParameter> &parameters, std::string &trouble)
{
    std::size_t k = at + 1;
    if (word::is_word_code(code_at(row, k))) {
        name = word::spelling_of(code_at(row, k));
        ++k;
    } else if (code_at(row, k) == token::name_token) {
        name = text_at(row, k);
    } else {
        return 0;
    }

    // ITS PARAMETERS, IF IT DECLARED ANY (2026-09-21). A type and then a name,
    // commas between, exactly the way every other declaration in satellite is
    // written. Anything else is recorded as trouble and refused by check_program
    // with a line to point at -- this scan has none, so it must not be the thing
    // that reports.
    if (code_at(row, k) == token::left_parenthesis_token) {
        ++k;
        if (code_at(row, k) != token::right_parenthesis_token) {
            for (;;) {
                if (!word::is_word_code(code_at(row, k))) {
                    trouble = "satellite.capsule " + name + " -- a parameter is a TYPE and "
                              "then a name, like " + name + "(satellite.variable.number n)";
                    break;
                }
                const Code declared = code_at(row, k);
                // THROUGH read_type_shape, so `satellite.container.list<satellite.variable.string>`
                // is one parameter and not a word followed by rubbish -- which is what
                // satellite.main has been declared with since 004's first program.
                TypeShape shape;
                unsigned int pending = 0;
                std::string unreadable;
                if (!read_type_shape(row, k, shape, pending, unreadable) || pending != 0) {
                    trouble = "satellite.capsule " + name + " -- " +
                              (unreadable.empty() ? std::string("there is a > here with nothing left for it to close")
                                                  : unreadable);
                    break;
                }
                if (code_at(row, k) != token::name_token) {
                    trouble = "satellite.capsule " + name + " -- " + std::string(word::spelling_of(declared)) +
                              " declares a name, and there is no name after it";
                    break;
                }
                const std::string spelled = text_at(row, k);
                parameters.push_back(CapsuleParameter{std::move(shape), spelled});
                if (code_at(row, k) != token::comma_token)
                    break;
                ++k;
            }
        }
        if (trouble.empty() && code_at(row, k) != token::right_parenthesis_token)
            trouble = "satellite.capsule " + name + " -- its ( is never closed on its line";
    }

    while (k < row.size() && code_at(row, k) != token::left_brace_token &&
           code_at(row, k) != token::right_brace_token) {
        if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
        ++k;
    }
    return code_at(row, k) == token::left_brace_token ? k : 0;
}

// The `{` a declaration's body opens with, from `at` onward: past the rest of its
// line and any line ends -- the author puts the brace on its own line. 0 when the
// next thing is not one.
std::size_t body_after(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (at < row.size() && code_at(row, at) != token::line_end_token &&
           code_at(row, at) != token::left_brace_token) {
        if (token::carries_a_count(code_at(row, at))) { skip_payload(row, at); continue; }
        ++at;
    }
    at = brace_after(row, at);
    return code_at(row, at) == token::left_brace_token ? at : 0;
}

// EVERY SCOPE AND CAPSULE OF ONE FILE. `open` is the scopes whose `{` has been read
// and whose `}` has not, the file's own at the bottom -- a stack, so a space may hold
// a space as deep as a person writes one, and no C++ recursion is spent on it.
void scan_row(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, const std::string &file)
{
    const std::size_t file_scope = table.scopes.size();
    CapsuleScope whole;
    whole.name = stem_of(file);
    whole.file = file;
    whole.row = r;
    whole.begins = 0;
    whole.ends = row.size();
    table.scopes.push_back(std::move(whole));
    table.file_scope.push_back(file_scope);
    table.in_row.emplace_back(1, file_scope);

    std::vector<std::size_t> open{file_scope};
    for (std::size_t i = 0; i < row.size();) {
        const Code code = code_at(row, i);
        // A PAYLOAD'S CODES ARE SKIPPED, NEVER CLASSIFIED: a string ending in U+1002
        // ends in 0x1002, and a character must never declare a capsule (the payload
        // sweep, 2026-09-17).
        const std::size_t here = open.back();
        const bool in_a_space = open.size() > 1;
        if (token::carries_a_count(code)) {
            // INSIDE A SPACE, A LINE THAT STARTS WITH A NAME, A NUMBER OR TEXT is
            // neither a capsule nor a space, and was skipped whole until the review
            // (2026-09-22) found `hello` and `42` accepted there. A character with no
            // code is still stepped over, one code, as a body steps over a no-break
            // space pasted as indentation.
            if (in_a_space && code != token::error_token) {
                refuse(table, r, i, satl_line_not_understood,
                       where_is(table.scopes[here]) + " holds capsules and other spaces, and this line is neither");
                i = past_the_statement(row, i);
                continue;
            }
            skip_payload(row, i);
            continue;
        }

        if (code == token::right_brace_token) {
            if (in_a_space) {
                table.scopes[here].ends = i;
                open.pop_back();
            }
            ++i;
            continue;
        }

        if (code == kCapsule) {
            std::string name, trouble;
            std::vector<CapsuleParameter> parameters;
            const std::size_t brace = capsule_header(row, i, name, parameters, trouble);
            if (brace == 0) { ++i; continue; }
            if (!trouble.empty())
                refuse(table, r, i, satl_line_not_understood, trouble);
            if (name == "satellite.main" && in_a_space)
                refuse(table, r, i, satl_line_not_understood,
                       "satellite.main goes at the top of its file, not inside " + where_is(table.scopes[here]) +
                           " -- it is where the program begins, and a space is reached by name from inside it");
            if (free_in(table, here, name, r, i, "capsule")) {
                const CapsuleScope &in = table.scopes[here];
                CapsuleSite site;
                site.row = r;
                site.body = brace + 1;
                site.parameters = std::move(parameters);
                site.scope = here;
                site.declared_at = i;
                site.name = name;
                const std::string written = in.within.empty() ? name : in.within + "." + name;
                site.shown = r == 0 ? written : table.scopes[file_scope].name + "." + written;
                site.key = std::to_string(r) + ":" + written;
                table.keys[site.key] = table.sites.size();
                table.scopes[here].capsules[name] = table.sites.size();
                table.sites.push_back(std::move(site));
            }
            // THE BODY IS STEPPED OVER WHOLE. What is inside a capsule is its
            // statements, and the checker judges those -- a satellite.capsule written
            // there is refused by it, by name, rather than quietly declared here.
            i = past_matching_brace(row, brace);
            continue;
        }

        if (code == kNamespace) {
            std::size_t k = i + 1;
            if (code_at(row, k) != token::name_token) {
                refuse(table, r, i, satl_line_not_understood,
                       "satellite.namespace needs a name -- satellite.namespace tools, and then its capsules "
                       "between { and }");
                i = past_the_statement(row, i);
                continue;
            }
            const std::string name = text_at(row, k);
            // A COMMENT MAY FOLLOW THE NAME, AND STAND ON ITS OWN LINES BEFORE THE `{`,
            // as it may before a capsule's. A comment is a MARKER, one code: its text
            // is not stored.
            while (code_at(row, k) == token::comment_token || code_at(row, k) == token::line_end_token)
                ++k;
            const std::size_t brace = k;
            if (code_at(row, brace) != token::left_brace_token) {
                refuse(table, r, i, satl_line_not_understood,
                       "satellite.namespace " + name + " has no body -- nothing but a comment may follow its name, "
                       "and its capsules go between a { and a } on the lines after it");
                i = past_the_statement(row, i);
                continue;
            }
            // A SECOND ONE OF A NAME IS STILL OPENED, so its braces balance and the
            // scan goes on to find anything else that is wrong -- it is only never
            // made reachable, and the program is refused either way.
            const bool named = free_in(table, here, name, r, i, "satellite.namespace");
            const std::size_t made = table.scopes.size();
            CapsuleScope space;
            space.name = name;
            space.within = table.scopes[here].within.empty() ? name : table.scopes[here].within + "." + name;
            space.parent = here;
            space.row = r;
            space.begins = brace + 1;
            space.ends = row.size();
            space.declared_at = i;
            table.scopes.push_back(std::move(space));
            if (named)
                table.scopes[here].spaces[name] = made;
            table.in_row.back().push_back(made);
            open.push_back(made);
            i = brace + 1;
            continue;
        }

        // A SPACESUIT HAS AN OBJECT AND NO GRAMMAR YET (MILESTONES M8). It is refused
        // by name and its body stepped over: before scopes, its capsules were read as
        // the FILE's, which is where "a capsule parameter is a user type" came from --
        // true, and about the wrong thing.
        if (code == kSpacesuit) {
            std::size_t k = i + 1;
            const std::string name = code_at(row, k) == token::name_token ? " " + text_at(row, k) : std::string();
            refuse(table, r, i, not_built_yet,
                   "satellite.spacesuit" + name + " is not built yet -- 004 has the spacesuit object and not "
                   "the grammar that declares one (MILESTONES M8), and it is the next piece after "
                   "satellite.namespace");
            const std::size_t brace = body_after(row, k);
            i = brace == 0 ? past_the_statement(row, i) : past_matching_brace(row, brace);
            continue;
        }

        // A BLOCK NOBODY DECLARED. Inside a space it is refused: stepping over it
        // hid whatever it held, a variable or a capsule, and nothing said so (the
        // review, 2026-09-22). At a file's top it is TRANSPARENT, as every block there
        // was before scopes -- a capsule written in one is still the file's, and its
        // `}` is left alone because only a space's `}` closes anything here.
        if (code == token::left_brace_token) {
            if (in_a_space) {
                refuse(table, r, i, satl_line_not_understood,
                       where_is(table.scopes[here]) + " holds capsules and other spaces, and this { opens neither");
                i = past_matching_brace(row, i);
            } else {
                ++i;
            }
            continue;
        }

        // AT A FILE'S TOP, EVERYTHING ELSE IS LEFT AS IT WAS: an include is read by
        // load_program and by the pass below, and anything more has never run.
        if (!in_a_space || code == token::line_end_token || code == token::comment_token) {
            ++i;
            continue;
        }

        // INSIDE A SPACE, ONLY CAPSULES AND SPACES -- the author, 2026-09-22.
        const CapsuleScope &in = table.scopes[here];
        if (code == kInclude)
            refuse(table, r, i, satl_line_not_understood,
                   "satellite.include goes at the top of the file, not inside " + where_is(in) +
                       " -- a space is part of its file, and the file's includes are its includes");
        else if (word::is_word_code(code) && code_at(row, i + 1) == token::name_token)
            refuse(table, r, i, satl_line_not_understood,
                   where_is(in) + " holds capsules and other spaces, not variables -- a variable belongs to a "
                                  "capsule or a spacesuit");
        else
            refuse(table, r, i, satl_line_not_understood,
                   where_is(in) + " holds capsules and other spaces, and this line is neither");
        i = past_the_statement(row, i);
    }

    // A SPACE THE FILE ENDS INSIDE. Its `ends` stays the row's end, so the scopes
    // still nest, and the innermost one is the one named.
    if (open.size() > 1) {
        const CapsuleScope &unclosed = table.scopes[open.back()];
        refuse(table, r, unclosed.declared_at, satl_line_not_understood,
               "satellite.namespace " + unclosed.name + " is never closed -- the file ends inside it, so its } "
               "is missing");
    }
}

// EVERY INCLUDE, JOINED TO THE ROW load_program READ IT INTO -- by the path it
// resolved to, which is the string load_program itself kept. And the rule 003 called
// S1602: a file's own capsule or space may not have the name a file it includes is
// reached by, or `tools.x()` could mean either.
void join_includes(CapsuleTable &table, const BytecodeRegistry &registry, const BytecodeFilenames &filenames)
{
    table.included.resize(registry.size());
    for (std::size_t r = 0; r < registry.size(); ++r) {
        const std::vector<std::bitset<16>> &row = registry[r];
        for (std::size_t i = 0; i < row.size();) {
            if (token::carries_a_count(code_at(row, i))) { skip_payload(row, i); continue; }
            if (code_at(row, i) != kInclude) { ++i; continue; }
            const std::size_t at = i;
            std::size_t k = i;
            const IncludeShape shape = include_at(row, k, filenames[r]);
            i = (k > i) ? k : i + 1;
            if (shape.kind == IncludeShape::Kind::none || shape.kind == IncludeShape::Kind::main_marker)
                continue;
            for (std::size_t q = 0; q < filenames.size(); ++q) {
                if (filenames[q] != shape.resolved)
                    continue;
                std::vector<std::size_t> &under = table.included[r][shape.name];
                const std::size_t theirs = table.file_scope[q];
                bool already = false;
                for (const std::size_t each : under) already = already || each == theirs;
                if (!already)
                    under.push_back(theirs);
                break;
            }
            const CapsuleScope &mine = table.scopes[table.file_scope[r]];
            const char *taken = mine.capsules.count(shape.name) != 0 ? "a capsule"
                                : mine.spaces.count(shape.name) != 0 ? "a satellite.namespace" : nullptr;
            if (taken != nullptr)
                refuse(table, r, at, name_declared_twice,
                       "the file " + shape.name + ".satl is reached as " + shape.name + ", and this file already has " +
                           taken + " named " + shape.name + " -- a name is declared once, so rename one of them");
        }
    }
}

} // namespace

CapsuleTable capsules_in(const BytecodeRegistry &registry, const BytecodeFilenames &filenames)
{
    CapsuleTable table;
    for (std::size_t r = 0; r < registry.size(); ++r)
        scan_row(table, registry[r], r, r < filenames.size() ? filenames[r] : std::string());
    join_includes(table, registry, filenames);
    return table;
}

} // namespace satellite004
