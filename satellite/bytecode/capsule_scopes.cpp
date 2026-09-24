// satellite/bytecode/capsule_scopes.cpp -- FINDING THE SCOPES. The header says what a
// scope is and whose each rule is. Two pieces here: the scan that finds every scope
// and capsule of a file, and the pass that joins each include to the row it loaded.
// capsule_reach.cpp is the other half: what a written name reaches.

#include "capsule_scan.hpp"

#include "include_shape.hpp"
#include "library_values.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"

#include <utility>

namespace satellite004 {
namespace scan {

using token::Code;

const Code kCapsule = word::code_of(1, 2);       // satellite.capsule
const Code kSpacesuit = word::code_of(1, 10);    // satellite.spacesuit, and satellite.class
const Code kInclude = word::code_of(1, 1);       // satellite.include
const Code kNamespace = word::code_of(1, 28);    // satellite.namespace, and satellite.space
const Code kReturns = word::code_of(1, 21);      // satellite.returns -- taken out, refused by name

std::string where_is(const CapsuleScope &scope)
{
    if (scope.parent == kNoScope)
        return "this file";
    return (scope.is_a_suit() ? "the spacesuit " : "the satellite.namespace ") + scope.within;
}

void refuse(CapsuleTable &table, std::size_t row, std::size_t at, signed long long int code, std::string why)
{
    table.troubles.push_back(ScopeTrouble{row, at, code, std::move(why)});
}

// A NAME IS DECLARED ONCE IN A SCOPE (the author, POLYMORPH D9.3: "a class declared
// twice is an ERROR: name collision"). True when `name` is free in `scope`; otherwise
// the refusal is recorded against the second declaration, which is the one to change.
// In a spacesuit its fields are names too: 003's S0515, "a spacesuit has one name for
// each of its fields and capsules".
bool free_in(CapsuleTable &table, std::size_t scope, const std::string &name, std::size_t row, std::size_t at,
             const char *declaring)
{
    const CapsuleScope &in = table.scopes[scope];
    const char *already = in.capsules.count(name) != 0 ? "a capsule"
                          : in.spaces.count(name) != 0 ? "a satellite.namespace"
                          : in.suits.count(name) != 0  ? "a spacesuit"
                          : (in.layout != nullptr && in.layout->slot_of(name) != kNoSlot) ? "a field"
                                                                                          : nullptr;
    if (already == nullptr)
        return true;
    refuse(table, row, at, name_declared_twice,
           name + " is declared twice in " + where_is(in) + " -- it is already " + already + " there, and this " +
               declaring + " would be a second thing with the same name");
    return false;
}

std::size_t header_rest(const std::vector<std::bitset<16>> &row, std::size_t k, const std::string &name,
                        std::vector<CapsuleParameter> &parameters, std::string &trouble)
{
    // ITS PARAMETERS, IF IT DECLARED ANY (2026-09-21). A type and then a name,
    // commas between, exactly the way every other declaration in satellite is
    // written. Anything else is recorded as trouble and refused by check_program
    // with a line to point at -- this scan has none, so it must not be the thing
    // that reports.
    // A HEADER MAY RUN OVER SEVERAL LINES, one parameter a line -- the author's
    // darkening.satl writes call_set_saidar_threads that way -- so a line end (or a
    // comment ending one) is stepped over wherever a parameter, a comma or the `)` may
    // stand, and nowhere else.
    const auto over_line_ends = [&row](std::size_t &at) {
        while (code_at(row, at) == token::line_end_token || code_at(row, at) == token::comment_token) ++at;
    };
    if (code_at(row, k) == token::left_parenthesis_token) {
        ++k;
        over_line_ends(k);
        if (code_at(row, k) != token::right_parenthesis_token) {
            for (;;) {
                over_line_ends(k);
                const Code declared = code_at(row, k);
                // A TYPE IS A WORD, OR A SPACESUIT'S NAME (2026-09-22): `run_log log`,
                // `tagged_report.run_log log`. read_type_shape reads both.
                if (!word::is_word_code(declared) && declared != token::name_token) {
                    trouble = name + " -- a parameter is a TYPE and then a name, like " + name +
                              "(satellite.variable.number n)";
                    break;
                }
                // THROUGH read_type_shape, so `satellite.container.list<satellite.variable.string>`
                // is one parameter and not a word followed by rubbish -- which is what
                // satellite.main has been declared with since 004's first program.
                TypeShape shape;
                unsigned int pending = 0;
                std::string unreadable;
                if (!read_type_shape(row, k, shape, pending, unreadable) || pending != 0) {
                    trouble = name + " -- " +
                              (unreadable.empty() ? std::string("there is a > here with nothing left for it to close")
                                                  : unreadable);
                    break;
                }
                if (code_at(row, k) != token::name_token) {
                    trouble = name + " -- " + shape_written(shape) + " declares a name, and there is no name after it";
                    break;
                }
                const std::string spelled = text_at(row, k);
                parameters.push_back(CapsuleParameter{std::move(shape), spelled});
                over_line_ends(k);
                if (code_at(row, k) != token::comma_token)
                    break;
                ++k;
            }
        }
        if (trouble.empty() && code_at(row, k) != token::right_parenthesis_token)
            trouble = name + " -- its ( is never closed on its line";

        // NOTHING AFTER ITS BRACKETS SAYS WHAT IT ANSWERS. satellite.returns(TYPE) was taken
        // out (the author, 2026-09-24: he never asked for it, in 003 or 004): a capsule
        // answers whatever its satellite.return(...) hands back. The word keeps its row in
        // words.tsv, whose numbering is append-only, so it still reads as itself -- and a
        // header that writes it is refused by name, with the one change to make.
        //
        // AND NOTHING ELSE STANDS THERE EITHER (2026-09-24). Before, whatever came between the
        // ) and the { was stepped over unread -- satellite.returns(...) on the next line, a
        // misspelled satellite.return(...), `42 "words" hello` -- so a header said things
        // satl never heard. Now only line ends and comments may come before the {.
        if (trouble.empty()) {
            std::size_t after = k + 1;
            over_line_ends(after);
            const Code next = code_at(row, after);
            if (next == kReturns)
                trouble = name + " -- satellite.returns was taken out of satellite: a capsule answers whatever its "
                                 "satellite.return(...) hands back, so delete satellite.returns(...) from this line";
            else if (next != token::left_brace_token && next != token::right_brace_token &&
                     next != token::end_of_file_token)
                trouble = name + " -- after its ) comes the { its body opens with, and something else stands "
                                 "between them";
        }
    }

    while (k < row.size() && code_at(row, k) != token::left_brace_token &&
           code_at(row, k) != token::right_brace_token) {
        if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
        ++k;
    }
    return code_at(row, k) == token::left_brace_token ? k : 0;
}

// ONE CAPSULE'S HEADER, from the satellite.capsule code at `at`: its name, then the
// rest (header_rest). Answers the brace's position, or 0 when this is not a
// declaration after all -- the same rule the scan had before scopes, and the same
// trouble sentences, which check_program now points a caret at.
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
    const std::size_t brace = header_rest(row, k, "satellite.capsule " + name, parameters, trouble);
    return brace;
}

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

void declare_capsule(CapsuleTable &table, std::size_t r, std::size_t file_scope, std::size_t here, std::size_t at,
                     std::size_t brace, std::string name, std::vector<CapsuleParameter> parameters,
                     Opened suit_part)
{
    if (!free_in(table, here, name, r, at, "capsule"))
        return;
    const CapsuleScope &in = table.scopes[here];
    CapsuleSite site;
    site.row = r;
    site.body = brace + 1;
    site.parameters = std::move(parameters);
    site.scope = here;
    site.declared_at = at;
    site.name = name;
    // A SPACESUIT'S CAPSULE RUNS ON ITS OBJECT, and is public only when written in its
    // satellite.public: 003 made a member written outside every section protected.
    if (in.is_a_suit()) {
        site.suit = here;
        site.is_public = suit_part == Opened::public_part;
    }
    const std::string written = in.within.empty() ? name : in.within + "." + name;
    site.shown = r == 0 ? written : table.scopes[file_scope].name + "." + written;
    site.key = std::to_string(r) + ":" + written;
    table.keys[site.key] = table.sites.size();
    table.scopes[here].capsules[name] = table.sites.size();
    table.sites.push_back(std::move(site));
}

namespace {

// EVERY SCOPE AND CAPSULE OF ONE FILE. `open` is what each `{` read and not yet
// closed is, the file's own at the bottom -- a stack, so a space may hold a space
// as deep as a person writes one, and no C++ recursion is spent on it.
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

    std::vector<Open> open{{file_scope, Opened::file}};
    bool line_begins = true;
    for (std::size_t i = 0; i < row.size();) {
        const Code code = code_at(row, i);
        const std::size_t here = open.back().scope;
        const bool in_a_space = open.back().what == Opened::space;
        // WHETHER THIS CODE BEGINS A LINE. A satellite.library value is a line of its own,
        // so a satellite.library met in the middle of some other line is not one.
        // A CHARACTER WITH NO CODE -- a no-break space pasted as indentation, a byte-order
        // mark -- changes nothing: the line still begins at the next code, as a body and a
        // spacesuit step over one (the review, 2026-09-23: a satellite.library line behind
        // one was neither recorded nor refused).
        const bool begins_a_line = line_begins;
        if (code != token::error_token)
            line_begins = code == token::line_end_token || code == token::comment_token;

        // A SPACESUIT'S BODY IS ITS OWN SET OF LINES (suit_scan.cpp): its sections, its
        // fields, its constructor, its capsules and the spacesuits inside it.
        if (in_a_suit(open.back())) {
            suit_line(table, row, r, file_scope, open, i);
            continue;
        }

        // A PAYLOAD'S CODES ARE SKIPPED, NEVER CLASSIFIED: a string ending in U+1002
        // ends in 0x1002, and a character must never declare a capsule (the payload
        // sweep, 2026-09-17).
        if (token::carries_a_count(code)) {
            // INSIDE A SPACE, A LINE THAT STARTS WITH A NAME, A NUMBER OR TEXT is
            // neither a capsule nor a space, and was skipped whole until the review
            // (2026-09-22) found `hello` and `42` accepted there. A character with no
            // code is still stepped over, one code, as a body steps over a no-break
            // space pasted as indentation.
            if (in_a_space && code != token::error_token) {
                refuse(table, r, i, satl_line_not_understood,
                       where_is(table.scopes[here]) + " holds capsules, spacesuits and other spaces, and this line "
                                                      "is none of them");
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
            declare_capsule(table, r, file_scope, here, i, brace, name, std::move(parameters), Opened::file);
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
            space.kind = ScopeKind::space;
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
            open.push_back({made, Opened::space});
            i = brace + 1;
            continue;
        }

        // A SPACESUIT (2026-09-22) -- at a file's top or in a space, as a capsule may be.
        if (code == kSpacesuit) {
            open_suit(table, row, r, file_scope, open, i);
            continue;
        }

        // A satellite.library VALUE (2026-09-23), written at a file's top and nowhere else:
        // `satellite.library.span = 25` (library_values.hpp).
        if (!in_a_space && begins_a_line && starts_a_library_line(code)) {
            library_line(table, row, r, file_scope, i);
            line_begins = true;      // library_line stepped past the line's end
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
                       where_is(table.scopes[here]) + " holds capsules, spacesuits and other spaces, and this { "
                                                      "opens none of them");
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

        // INSIDE A SPACE, ONLY CAPSULES, SPACESUITS AND SPACES -- the author, 2026-09-22.
        const CapsuleScope &in = table.scopes[here];
        if (code == kInclude)
            refuse(table, r, i, satl_line_not_understood,
                   "satellite.include goes at the top of the file, not inside " + where_is(in) +
                       " -- a space is part of its file, and the file's includes are its includes");
        else if (starts_a_library_line(code))
            refuse(table, r, i, satl_line_not_understood,
                   "a satellite.library value is written at the top of its file, not inside " + where_is(in) +
                       " -- a file has one satellite.library, and every capsule in it reads the same values");
        else if (word::is_word_code(code) && code_at(row, i + 1) == token::name_token)
            refuse(table, r, i, satl_line_not_understood,
                   where_is(in) + " holds capsules, spacesuits and other spaces, not variables -- a variable belongs "
                                  "to a capsule or a spacesuit");
        else
            refuse(table, r, i, satl_line_not_understood,
                   where_is(in) + " holds capsules, spacesuits and other spaces, and this line is none of them");
        i = past_the_statement(row, i);
    }

    // A SPACE OR A SPACESUIT THE FILE ENDS INSIDE. Its `ends` stays the row's end, so
    // the scopes still nest, and the innermost one is the one named.
    if (open.size() > 1) {
        const CapsuleScope &unclosed = table.scopes[open.back().scope];
        refuse(table, r, unclosed.declared_at, satl_line_not_understood,
               std::string(unclosed.is_a_suit() ? "satellite.spacesuit " : "satellite.namespace ") + unclosed.name +
                   " is never closed -- the file ends inside it, so its } is missing");
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
                                : mine.spaces.count(shape.name) != 0 ? "a satellite.namespace"
                                : mine.suits.count(shape.name) != 0  ? "a spacesuit"
                                : mine.library.count(library_name_of(shape.name)) != 0 ? "a satellite.library value"
                                                                                        : nullptr;
            if (taken != nullptr)
                refuse(table, r, at, name_declared_twice,
                       "the file " + shape.name + ".satl is reached as " + shape.name + ", and this file already has " +
                           taken + " named " + shape.name + " -- a name is declared once, so rename one of them");
        }
    }
}

} // namespace
} // namespace scan

CapsuleTable capsules_in(const BytecodeRegistry &registry, const BytecodeFilenames &filenames)
{
    CapsuleTable table;
    for (std::size_t r = 0; r < registry.size(); ++r)
        scan::scan_row(table, registry[r], r, r < filenames.size() ? filenames[r] : std::string());
    scan::join_includes(table, registry, filenames);
    // A SPACESUIT'S NAME WRITTEN AS A TYPE reaches its scope only once every file is
    // scanned and joined -- `tagged_report.run_log` names a spacesuit of another file.
    scan::resolve_types(table);
    return table;
}

} // namespace satellite004
