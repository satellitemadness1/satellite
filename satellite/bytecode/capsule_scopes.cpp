// satellite/bytecode/capsule_scopes.cpp -- FINDING THE SCOPES. The header says what a
// scope is and whose each rule is. Two pieces here: the scan that finds every scope
// and capsule of a file, and the pass that joins each include to the row it loaded.
// capsule_reach.cpp is the other half: what a written name reaches.

#include "capsule_scan.hpp"

#include "include_shape.hpp"
#include "library_values.hpp"
#include "../machine/source_position.hpp"
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
const Code kMain = word::code_of(1, 3);          // satellite.main
const Code kReturn = word::code_of(1, 15);       // satellite.return

const char *const kOutsideEveryCapsule =
    "this line is outside every capsule, and nothing outside a capsule ever runs -- there are no globals in "
    "satellite, so it goes inside satellite.main or another capsule";

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

// WHERE LINE `line` (0-based) BEGINS IN THE ROW: its first code. The lexer ends every
// line with a line_end_token, and a payload's codes are skipped, never counted.
std::size_t first_code_of_line(const std::vector<std::bitset<16>> &row, std::size_t line)
{
    std::size_t at = 0;
    for (std::size_t seen = 0; seen < line && at < row.size();) {
        if (token::carries_a_count(code_at(row, at))) { skip_payload(row, at); continue; }
        if (code_at(row, at) == token::line_end_token) ++seen;
        ++at;
    }
    return at;
}

// A BODY THE FILE ENDS INSIDE: WHICH { WAS LEFT OPEN, when a list's is the likely one --
// a { where a value goes (after ( , = [ or another {) with fewer } after it on its own
// line. `display({1, 2)` is the shape: the list's { then takes the capsule's own }, and
// the capsule is the one that looks unclosed. Asked only of a body that is already
// broken, so it chooses a sentence and never refuses a program that works. The last
// code seen is carried forward, never looked back at: a payload's last code can be
// any 16 bits.
std::size_t unclosed_list_in(const std::vector<std::bitset<16>> &row, std::size_t from)
{
    Code before = 0;
    for (std::size_t at = from; at < row.size();) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { before = code; skip_payload(row, at); continue; }
        if (code == token::left_brace_token &&
            (before == token::left_parenthesis_token || before == token::comma_token ||
             before == token::assign_token || before == token::left_square_bracket_token ||
             before == token::left_brace_token)) {
            long depth = 0;
            for (std::size_t k = at; k < row.size() && code_at(row, k) != token::line_end_token &&
                                     code_at(row, k) != token::end_of_file_token;) {
                if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
                if (code_at(row, k) == token::left_brace_token) ++depth;
                else if (code_at(row, k) == token::right_brace_token) --depth;
                ++k;
            }
            if (depth > 0)
                return at;
        }
        before = code;
        ++at;
    }
    return std::string::npos;
}

// ...AND WHERE THE FILE GOES ON: the next line that begins with satellite.capsule,
// satellite.spacesuit or satellite.namespace, none of which can stand inside a capsule --
// so the scan carries on from there and the capsules after a missing } are still
// declared, and a call to one is not refused as "no capsule named" first. The row's end
// when there is none.
std::size_t next_declaration_after(const std::vector<std::bitset<16>> &row, std::size_t from)
{
    bool line_begins = false;
    for (std::size_t at = from; at < row.size();) {
        const Code code = code_at(row, at);
        if (line_begins && (code == kCapsule || code == kSpacesuit || code == kNamespace))
            return at;
        if (code != token::error_token)
            line_begins = code == token::line_end_token || code == token::comment_token;
        if (token::carries_a_count(code)) { skip_payload(row, at); continue; }
        ++at;
    }
    return row.size();
}

// A STRING WITH NO CLOSING QUOTE (the error sweep, 2026-09-25). The lexer lets it run to
// the line's end and records nothing, so `display("hello)` was refused only when it RAN
// -- after the lines above it had printed -- as something "it could not read to the end
// of", and `s = "abc` was quietly "abc". The author's own quad_main.satl line 6 is the
// first shape. Read from the loaded text by the lexer's own two rules -- a `//` outside a
// string ends the line, and inside one a backslash takes the character after it -- so
// the two cannot disagree about where a string ends.
void refuse_unclosed_strings(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r,
                             const std::string &file)
{
    const std::unordered_map<std::string, std::string>::const_iterator loaded = loaded_sources().find(file);
    if (loaded == loaded_sources().end())
        return;
    const std::string &text = loaded->second;
    std::size_t line = 0;
    std::size_t from = 0;
    if (text.compare(0, 2, "#!") == 0) {                // a shebang is not satellite's to read
        from = text.find('\n');
        if (from == std::string::npos) return;
        ++from;
        line = 1;
    }
    for (std::size_t start = from; start <= text.size(); ++line) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        for (std::size_t k = start; k < end;) {
            if (text[k] == '/' && k + 1 < end && text[k + 1] == '/') break;
            if (text[k] != '"') { ++k; continue; }
            ++k;
            while (k < end && text[k] != '"') k += (text[k] == '\\' && k + 1 < end) ? 2 : 1;
            if (k >= end) {
                refuse(table, r, first_code_of_line(row, line), satl_line_not_understood,
                       "a string on this line has no closing \" -- a string begins and ends with \" on the "
                       "same line, and a \" inside one is written \\\"");
                break;
            }
            ++k;
        }
        if (end == text.size()) break;
        start = end + 1;
    }
}

// A SHEBANG, `#!/usr/bin/env satl`, AS A FILE'S FIRST LINE: what makes a program a script a
// shell can run, and stepped over like a comment. Every line at the top was stepped over
// before 2026-09-25, and refusing them all must not take this one away (the review).
bool starts_with_a_shebang(const std::string &file)
{
    const std::unordered_map<std::string, std::string>::const_iterator loaded = loaded_sources().find(file);
    return loaded != loaded_sources().end() && loaded->second.compare(0, 2, "#!") == 0;
}

// WHAT A REFUSED TOP-LEVEL LINE IS TOLD. A comment written the way another language writes
// one -- `#`, `/*`, or a ` * ` line inside one -- is told that satellite's comments begin
// with //, where "outside every capsule" sent a person looking for a capsule (the review
// of the error sweep, 2026-09-25).
std::string outside_every_capsule(const std::vector<std::bitset<16>> &row, std::size_t at, const std::string &file)
{
    std::size_t line = 1;
    for (std::size_t k = 0; k < at && k < row.size();) {
        if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
        if (code_at(row, k) == token::line_end_token) ++line;
        ++k;
    }
    const std::string text = source_line(file, line);
    const std::size_t first = text.find_first_not_of(" \t");
    if (first != std::string::npos && (text[first] == '#' || text[first] == '*' || text.compare(first, 2, "/*") == 0))
        return "satellite's comments begin with // -- a line that starts with # or /* is not a comment in satellite";
    return kOutsideEveryCapsule;
}

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

    refuse_unclosed_strings(table, row, r, file);

    std::vector<Open> open{{file_scope, Opened::file}};
    // THE {s OPENED AT THE FILE'S TOP BY NO DECLARATION, still open -- where each is, so a
    // } that closes nothing and a { never closed can both be refused (2026-09-25).
    std::vector<std::size_t> top_blocks;
    bool line_begins = true;
    for (std::size_t i = starts_with_a_shebang(file) ? first_code_of_line(row, 1) : 0; i < row.size();) {
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
            //
            // AT A FILE'S TOP THE SAME LINE IS OUTSIDE EVERY CAPSULE -- `greet()`, `x = 5`
            // -- and is refused for that below, in the same words (2026-09-25).
            if ((in_a_space || begins_a_line) && code != token::error_token) {
                refuse(table, r, i, satl_line_not_understood,
                       in_a_space ? where_is(table.scopes[here]) +
                                        " holds capsules, spacesuits and other spaces, and this line is none of them"
                                  : outside_every_capsule(row, i, file));
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
            } else if (!top_blocks.empty()) {
                top_blocks.pop_back();
            } else {
                // A } THAT CLOSES NOTHING -- one too many after a capsule -- was stepped
                // over and the program ran (the error sweep, 2026-09-25).
                refuse(table, r, i, satl_line_not_understood,
                       "this } closes nothing -- every { above it is already closed, so it is one } too many");
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
            const std::size_t capsule_at = i;
            i = past_matching_brace(row, brace);
            // A BODY THE FILE ENDS INSIDE: its } -- or one inside it -- is missing, and the
            // program ran as though the end of the file closed it (the error sweep,
            // 2026-09-25). A matched } is never the row's last code: end_of_file_token is.
            if (i >= row.size()) {
                const std::size_t list = unclosed_list_in(row, brace + 1);
                if (list != std::string::npos) {
                    // expression.cpp's own sentence for it, said now rather than when it runs.
                    refuse(table, r, list, satl_line_not_understood,
                           "this list was opened with { and never closed with } -- items are separated by commas, "
                           "as in {\"one\", \"two\"}");
                } else {
                    refuse(table, r, capsule_at, satl_line_not_understood,
                           "satellite.capsule " + name + " is never closed -- the file ends inside it, so a } is "
                           "missing: its own, or one inside it");
                    table.troubles.back().after_the_bodies = true;
                }
                i = next_declaration_after(row, brace + 1);
                line_begins = true;
            }
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
                top_blocks.push_back(i);
                ++i;
            }
            continue;
        }

        if (code == token::line_end_token || code == token::comment_token) {
            ++i;
            continue;
        }

        // AT A FILE'S TOP, A LINE IS AN INCLUDE, A satellite.return, OR ONE OF THE
        // DECLARATIONS ABOVE -- and nothing else. Anything more was stepped over here
        // and never run, and nothing said so: `satellite.console.display("hi")` above
        // main printed nothing and the program exited 0 (the help writers, 2026-09-22;
        // refused 2026-09-25). The author's own rule is why it cannot run instead:
        // "THERE ARE NO GLOBALS IN SATELLITE, we begin exe inside of main, and end exe
        // inside of main... the only globals are the includes, other files"
        // (include_shape.hpp). So it is refused, before anything runs, by name.
        //
        // AN INCLUDE THAT NAMES NO FILE is refused here too: include_at found none of
        // the five spellings, and load_program and join_includes both step over that,
        // so the program ran without whatever was meant (include(./x) did, until it
        // became a spelling on the same day).
        if (!in_a_space) {
            if (!begins_a_line || code == token::error_token || code == token::end_of_file_token) {
                ++i;                                // the rest of a line already judged
                continue;
            }
            if (code == kInclude) {
                std::size_t k = i;
                if (include_at(row, k, file).kind == IncludeShape::Kind::none)
                    refuse(table, r, i, satl_line_not_understood,
                           "this satellite.include names no file -- write satellite.include(ship), "
                           "satellite.include(parts/ship) or satellite.include(\"parts/ship.satl\")");
                ++i;
                continue;
            }
            if (code == kReturn) {                  // satellite.return(satellite), where a file ends
                ++i;
                continue;
            }
            if (code == kMain)
                refuse(table, r, i, satl_file_missing_satellite_main,
                       "satellite.main is a capsule, so it is declared like one -- write satellite.capsule "
                       "satellite.main()");
            else
                refuse(table, r, i, satl_line_not_understood, outside_every_capsule(row, i, file));
            i = past_the_statement(row, i);
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

    if (!top_blocks.empty())
        refuse(table, r, top_blocks.back(), satl_line_not_understood,
               "this { is never closed -- the file ends inside it, so its } is missing");

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
