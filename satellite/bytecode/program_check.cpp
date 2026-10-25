// satellite/bytecode/program_check.cpp -- NOTHING RUNS BEFORE THE WHOLE PROGRAM
// IS CHECKED. Every capsule body is walked and every statement judged before
// main is entered, so a program that cannot finish does not half-print first --
// which check.sh asserts in as many words ("nothing ran before the refusal").
//
// IT CHECKS SHAPE AND NAMES, NOT TYPES, and the line between those is the point
// of this file. A statement's SHAPE is knowable without running: whether a word
// is a call or a declaration, whether a library exists for it, whether a name
// was ever declared, whether a while has a body. A statement's TYPES are not --
// `n = a + b` depends on what a and b hold, and working that out here would mean
// running the program to check the program. So the kinds are judged where they
// are known, at the moment the operator meets them (expression.cpp), and this
// pass guarantees only that every line is one the walker recognises.
//
// WHAT THAT BUYS, EXACTLY: a program whose fifth line names a word with no
// library, or a variable nothing declared, prints nothing at all rather than
// four lines and then a refusal. What it does not buy is catching `1 + "a"` in an
// unrun branch, and this file does not pretend to.
//
// TWO TYPE RULES ARE CHECKED HERE, and only because each is a SPELLING and not a
// type: a satellite.variable.binary given digits with no b in front of them (the
// author, 2026-09-16), and a satellite.variable.percentage given digits with no %
// after them. The b and the % are visible in the text, so neither needs anything
// to run -- see binary_is_written_with_b and percentage_is_written_with_percent.
//
// THE DECLARED NAMES ARE TRACKED PER CAPSULE, which is the same rule run_body
// enforces by handing each body its own table: there are no globals, so a name
// declared in one capsule is not declared in another.

#include "program_walk.hpp"

#include "suit_run.hpp"
#include "file_calls.hpp"
#include "info_calls.hpp"
#include "convert_calls.hpp"
#include "fraction_check.hpp"
#include "access_calls.hpp"
#include "argument_switches.hpp"
#include "color_values.hpp"
#include "console_calls.hpp"
#include "container_calls.hpp"
#include "pointer_calls.hpp"
#include "string_calls.hpp"
#include "number_calls.hpp"
#include "method_periods.hpp"
#include "main_arguments.hpp"
#include "setting_writes.hpp"
#include "../arguments/argument_settings.hpp"
#include "../arguments/arguments.hpp"
#include "float_values.hpp"
#include "fraction_values.hpp"
#include "hexadecimal_values.hpp"
#include "infinity_calls.hpp"
#include "library_values.hpp"
#include "random_calls.hpp"
#include "time_calls.hpp"
#include "drive_calls.hpp"
#include "test_calls.hpp"
#include "window_calls.hpp"
#include "window_readers.hpp"
#include "thread_calls.hpp"
#include "program_calls.hpp"
#include "word_codes.hpp"
#include "delete_calls.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/source_position.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace satellite004 {
namespace {

using token::Code;
// Each declared name and the word that declared it -- the TYPE is kept so that a
// later `bits = 1010` can be judged by the same rule as the declaration was.
//
// SINCE 2026-10-07 THIS IS satellite.legal'S OWN LIST FOR THE PLACE BEING READ (satellite_legal.hpp's
// LegalNames), and no longer a map of the checker's own that the master list only mirrored: a name found here
// is one its place holds as legal on this line, a name declared is written into the place, and a name a }
// ends is ended there -- the author's "we ourselves will use satellite.legal for checking whether operations
// are legal or not". The shape is the map's, so every judgement below reads as it did.
using DeclaredNames = LegalNames;

// AND EACH NAME THAT HOLDS AN OBJECT, WITH THE SPACESUIT IT IS AN OBJECT OF (2026-09-22).
// Beside DeclaredNames rather than in it, because five other files are handed that list
// and none of them has any business with spacesuits.
using DeclaredObjects = std::unordered_map<std::string, std::size_t>;

inline constexpr std::size_t kNowhere = static_cast<std::size_t>(-1);

// THE NAME RULE, AS THE LEXER KEEPS IT (DESIGN §7, bytecode_registry.cpp's
// identifier_start): said in the two refusals a name that broke it lands in (M5).
const std::string kNameRule = "a name is made of a-z, A-Z, 0-9 and _, and does not start with a digit";
constexpr std::size_t kNoStatement = static_cast<std::size_t>(-1);

// HABITS FROM ANOTHER LANGUAGE, said as what to write here (the error sweep, 2026-09-25),
// for a person meeting satellite for the first time. Each shape was refused already, in
// a sentence about something else: 'hello' as "hello has no satellite.variable line
// declaring it", a closing ; as "followed by something that is not a method call". A
// single quote means nothing outside a string, and a ; only inside a for's brackets, so
// neither can be a program that works. The last code is carried forward, never looked
// back at: a payload's last code can be any 16 bits.
bool habit_from_another_language(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &why)
{
    Code last = 0;
    for (std::size_t k = at; k < row.size();) {
        const Code code = code_at(row, k);
        if (code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token)
            break;
        if (token::carries_a_count(code)) { last = code; skip_payload(row, k); continue; }
        if (code == token::single_quote_token) {
            why = "text goes between double quotes in satellite -- write \"hello\", not 'hello'";
            return true;
        }
        last = code;
        ++k;
    }
    if (last == token::semicolon_token) {
        why = "a line needs no ; at its end in satellite -- one statement is one line, and the line's end ends it";
        return true;
    }
    return false;
}

// WHY A LINE CANNOT BEGIN WITH THIS CODE (ERRORS4 003): a value, a sign or a closing bracket, which check_statement
// has no arm for -- said as the sign line's sentence is, "a line does not begin with ...".
std::string a_line_does_not_begin_with(Code code)
{
    if (code == token::comment_start_token || code == token::comment_end_token)
        return std::string("a line does not begin with ") + (code == token::comment_start_token ? "/*" : "*/") +
               " -- a comment is written //, and runs to the end of its line";
    if (token::carries_a_count(code))
        return "a line does not begin with a value -- a value on a line of its own does nothing: give it to a name, "
               "n = 5";
    if (code == token::method_token || code == token::satellite_method_token)
        return "a line does not begin with a . -- a .method goes on its value's line, or on the very next line";
    const char *sign = code == token::not_token                   ? "!"
                     : code == token::right_parenthesis_token     ? ")"
                     : code == token::right_square_bracket_token  ? "]"
                     : code == token::left_parenthesis_token      ? "("
                     : code == token::left_square_bracket_token   ? "["
                     : code == token::equals_token                ? "=="
                     : code == token::not_equals_token            ? "!="
                     : code == token::less_than_token             ? "<"
                     : code == token::greater_than_token          ? ">"
                     : code == token::less_or_equal_token         ? "<="
                     : code == token::greater_or_equal_token      ? ">="
                     : code == token::assign_token                ? "="
                     : code == token::times_token || code == token::tight_times_token ? "*"
                     : code == token::divide_token || code == token::path_separator_token ? "/"
                     : code == token::modulus_token || code == token::tight_modulus_token ? "%"
                     : code == token::power_token || code == token::tight_power_token ? "^"
                     : code == token::comma_token                 ? ","
                                                                  : nullptr;
    return std::string("a line does not begin with ") + (sign != nullptr ? sign : "this") +
           " -- a line begins with a name or a word of the language";
}

// HOW MANY ONE-CHARACTER CHANGES TURN a INTO b (Levenshtein), for a misspelled word.
std::size_t changes_between(const std::string &a, const std::string &b)
{
    std::vector<std::size_t> above(b.size() + 1), here(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) above[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        here[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j)
            here[j] = std::min({above[j] + 1, here[j - 1] + 1, above[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
        std::swap(above, here);
    }
    return above[b.size()];
}

// THE WORD A MISSPELLING MOST LIKELY MEANT, "" when none is close: the words under
// `known` (satellite.console) whose next part is at most two changes from `typed`
// (dispaly), the nearest first -- so satellite.console.dispaly is asked "did you mean
// satellite.console.display?" rather than told satellite.console is not a call, which
// named the part that was right (the error sweep, 2026-09-25).
std::string word_it_most_likely_meant(const std::string &known, const std::string &typed)
{
    const std::string under = known + ".";
    std::string best;
    std::size_t nearest = 3;
    for (std::size_t n = 0; n < word::kSpelledWordCount; ++n) {
        const std::string_view path = word::kSpelledWords[n].path;
        if (path.size() <= under.size() || path.compare(0, under.size(), under) != 0)
            continue;
        const std::string next(path.substr(under.size(), path.find_first_of(".(", under.size()) - under.size()));
        const std::size_t changes = changes_between(next, typed);
        if (changes > 0 && changes < nearest) {
            nearest = changes;
            best = under + next;
        }
    }
    return best;
}

// WHERE A STATEMENT STANDS: the scope table and the scope, the capsule whose body it is,
// and what that body has declared that holds an object. One of these a body, handed to
// every judgement of it -- it replaced the three separate arguments every one of them took.
struct Where {
    Where(const BytecodeRegistry &its_registry, const CapsuleTable &its_capsules, std::size_t its_scope,
          const FunctionTable &its_functions)
        : registry(its_registry), capsules(its_capsules), scope(its_scope), functions(its_functions) {}

    const BytecodeRegistry &registry;
    const CapsuleTable &capsules;
    std::size_t scope;
    const FunctionTable &functions;
    const CapsuleSite *site = nullptr;   // the capsule whose body this is; null for a field, and at the prompt
    bool field = false;                  // a spacesuit's field: its value is worked out before there is an object
    // EVERY NAME THE PROGRAM GIVES A WIDGET, in any of its files (the author's .widgets, 2026-10-09):
    // `my_window.a_name` is judged against them before anything runs. Null at the prompt, where a
    // line made on an earlier line is not here to read, and the walker judges the name when it runs.
    const std::unordered_set<std::string> *widget_names = nullptr;
    DeclaredObjects objects;
    // AND EACH LIST DECLARED TO HOLD A SPACESUIT'S OBJECTS, with that spacesuit -- so
    // `units[i].call_x()` is judged by what the list was declared to hold.
    DeclaredObjects lists;
    // AND EACH multiple WITH ONE SPACESUIT AMONG ITS TYPES, with that spacesuit: asked only
    // for a capsule after its dot, since its container methods are a container's (remember_shape).
    DeclaredObjects multiples;
    // AND EACH NAME'S WHOLE DECLARED SHAPE (2026-10-10, the review of `.satellite.`): what `names[1]` is, for
    // the period after it -- `list<string> names` makes `names[1].size` a string's word, told before anything
    // runs. Read only while the name's type word still says the same, so a name declared again is never judged
    // by its old shape.
    std::unordered_map<std::string, TypeShape> shapes;
    // satellite.statement.break AND .continue NEED A LOOP AROUND THEM (2026-09-26). A while or a
    // for leaves its `{` marked as a loop's; the body loops keep one mark an open brace.
    bool opening_a_loop = false;
    // THE `{` AN if, else, while OR for FOUND AFTER ITS HEADER -- the only `{` a line may begin with
    // (a_brace_that_opens_nothing, below).
    std::size_t body_brace = kNowhere;
    std::vector<bool> open_blocks;
    // satellite.error() (ERROR_BLOCK_PLAN.md): the `{` an error block found after its header, so the body loop marks
    // its block as one; where the last if's, else's, while's or for's `}` stood -- the one place an error block may
    // follow -- and where the last error block's `}` stood, which an else may not follow.
    std::size_t error_brace = kNowhere;
    std::vector<bool> error_blocks;
    std::size_t block_closed_at = kNowhere;
    std::size_t error_closed_at = kNowhere;
    // AND THE `{` AN if FOUND AFTER ITS HEADER -- an else if's too -- marked the same way, and where the last if's `}`
    // stood: the one `}` an else may follow (ERRORS4 027). A while's `}`, or an else's, let an else through to the
    // run, which refused it after the lines above it had printed.
    std::size_t if_brace = kNowhere;
    std::vector<bool> if_blocks;
    std::size_t if_closed_at = kNowhere;
    void error_block_opened(std::size_t brace)
    {
        error_blocks.push_back(brace == error_brace);
        if (brace == error_brace) error_brace = kNowhere;
        if_blocks.push_back(brace == if_brace);
        if (brace == if_brace) if_brace = kNowhere;
    }
    void error_block_closed(std::size_t brace)
    {
        const bool an_error_block = !error_blocks.empty() && error_blocks.back();
        if (!error_blocks.empty()) error_blocks.pop_back();
        block_closed_at = an_error_block ? kNowhere : brace;
        error_closed_at = an_error_block ? brace : kNowhere;
        const bool an_if = !if_blocks.empty() && if_blocks.back();
        if (!if_blocks.empty()) if_blocks.pop_back();
        if_closed_at = an_if ? brace : kNowhere;
    }
    bool inside_a_loop() const
    {
        for (const bool loop : open_blocks) if (loop) return true;
        return false;
    }
    // satellite.delete(name) STANDS IN THE BLOCK THAT DECLARED THE NAME (delete_calls.hpp). Each
    // block opened takes the next number, the body's own level is 0, and a name declared inside a
    // block keeps that block's number in `declared_in` -- a name with none there was declared at the
    // body's own level: a parameter, a field, a name the prompt kept.
    std::vector<std::size_t> block_numbers;
    std::size_t blocks_opened = 0;
    std::size_t this_block() const { return block_numbers.empty() ? 0 : block_numbers.back(); }
    std::unordered_map<std::string, std::size_t> declared_in;
    // AND, FOR EACH NAME satellite.delete TOOK, THE BLOCK IT WENT IN AND THE LINE A PERSON WROTE THAT
    // DELETE ON -- for the rest of this check only. A deleted name is free again only inside the block it
    // went in (deleted_in_another_block); and a later line that names it is told the line it went on -- it
    // WAS declared, above the delete, so "nothing declared it" would be untrue.
    std::unordered_map<std::string, std::size_t> deleted_in;
    std::unordered_map<std::string, std::size_t> deleted_on_line;
    // satellite.legal (satellite_legal.hpp): the place this body is in the master list, and whether a name ends
    // at the } of the block that declared it -- arguments.access on, the default -- or is legal to the end of the
    // capsule. `ended_at` is where the } stood that ended each name, read only when a later line is refused.
    LegalPlace *place = nullptr;
    bool names_end_with_their_block = true;
    std::unordered_map<std::string, std::size_t> ended_at;
    // AT THE PROMPT, A KEPT NAME WHOSE BLOCK ENDED ON AN EARLIER LINE: its record is kept, inaccessible.
    std::unordered_map<std::string, bool> ended_earlier;
    void opened() { open_blocks.push_back(opening_a_loop); opening_a_loop = false; block_numbers.push_back(++blocks_opened); }
    void closed()
    {
        if (!open_blocks.empty()) open_blocks.pop_back();
        if (!block_numbers.empty()) block_numbers.pop_back();
    }
    std::size_t statement = kNowhere;    // where the statement being judged starts
    // WHERE A for's STEP BEGINS, while its condition and step are judged: a touching sign there is
    // for_step_moves_by's to judge -- i++ is right -- and names_in_statement leaves it alone (ERRORS4 022).
    std::size_t for_step = kNowhere;
    const Arguments *arguments = nullptr;  // the rows satl holds, which `argz.row = x` may not write
    bool typed_line = false;             // at the prompt: declaring a kept name again replaces it
    // AT THE PROMPT, FOR THE ARGUMENTS (2026-10-06): a setting a line there changes is saved for good. A typed
    // line, AND a capsule declared at the prompt, which check_program judges as a file's -- the walker runs it
    // at the prompt, and the two must agree (the fresh reader, 2026-10-06). From MachineState::at_the_prompt.
    bool at_the_prompt = false;
    // AND THE KEPT NAMES THIS LINE DECLARES AGAIN, with their new types, taken only once
    // the line has been judged (check_typed_line): its value is worked out from the OLD
    // one, as the walker works it out, so `satellite.variable.string words =
    // words.join(",")` over a kept list is judged as the list's join.
    std::vector<std::pair<std::string, Code>> replacing;
    // A SECTION'S OR A DISPLAY'S NAME IS A NAME (SATELLITE_WINDOW_SECTIONS.md SEC-0, rule 4): each name such a line
    // declared here, with what made it -- "section", "file display", "text input" -- for the sentence a second
    // declaration of it is told (declared_twice). Forgotten when the name is declared afresh (declares).
    std::unordered_map<std::string, std::string> made_by;
    // AND WHERE A DISPLAY OR A SECTION MAY GO (rules 6 and 7), judged before the run only where the checker can see
    // the lines: what is in each window, section and display whose lines run STRAIGHT DOWN this capsule -- at the
    // body's own level, in no block, which `straight_down` says of the line being judged (check_program's body loop
    // sets it; a typed line and a field are never straight down). A window is seen from the line that declares it
    // from satellite.window.new, and a section or a display from its own line. A name stops being seen -- with
    // everything made in it -- the moment a line uses it as anything but the receiver of its own line straight down:
    // handed to a capsule, put in a list, given to another name, given a value, closed, or written in a block. What
    // is not seen is refused when its line runs, by window_widget_made (where_it_goes_refused says the rest).
    struct SeenWhole {
        bool a_window = false;       // from satellite.window.new; otherwise a section or a display
        bool a_section = false;
        bool holds_sections = false;
        bool shows_a_display = false;   // a section: the one that fills it; a window: one across its top
        std::string made_in;         // the name of what it was made in, when that was a name
    };
    std::unordered_map<std::string, SeenWhole> seen_whole;
    bool straight_down = false;
};

// A `{` THAT OPENS NO BODY, refused before anything runs (the author, 2026-10-12: "fix this as the next thing that
// you do", on his "refuse yes" of 2026-10-11). The walker steps over a code it has no arm for, so `{1, 2}` on a line
// of its own ran as nothing and its `}` ended the capsule there -- every line after it skipped, exit 0 -- and a bare
// { ... } ended it the same way at its `}`. A line begins with { only where an if, else, while or for opened a body.
bool a_brace_that_opens_nothing(Where &where, std::size_t at, std::string &why)
{
    if (at == where.body_brace) {
        where.body_brace = kNowhere;
        return false;
    }
    why = "a line does not begin with { here -- a { opens the body of satellite.statement.if, else, while or for, "
          "and a list on a line of its own does nothing: give it to a name, "
          "satellite.container.list<satellite.variable.number> a = {1, 2}";
    return true;
}

// THE FILE A STATEMENT STANDS IN, as load_program read it -- for reading back what it wrote.
// A line typed at the prompt with nothing declared around it is session.cpp's "<typed>".
std::string file_of(const Where &where)
{
    std::size_t s = where.scope;
    while (s < where.capsules.scopes.size() && where.capsules.scopes[s].kind != ScopeKind::file)
        s = where.capsules.scopes[s].parent;
    if (s < where.capsules.scopes.size()) return where.capsules.scopes[s].file;
    return where.typed_line ? std::string("<typed>") : std::string();
}

// A NAME satellite.delete TOOK INSIDE A BLOCK IS FREE AGAIN ONLY IN THAT BLOCK, after its delete --
// anywhere else, declaring it again is declaring it twice, as it was before satellite.delete: an else
// beside that block, or a line after it, would leave whether the name is there depending on which block
// ran (the fresh reader, 2026-10-06). A name deleted at the body's own level is free from there on.
bool deleted_in_another_block(const Where &where, const std::string &name)
{
    // WHILE NAMES END AT THEIR BLOCK it never arises: a name deleted inside a block has ended at its } anyway,
    // and one declared after it is a new name.
    if (where.names_end_with_their_block)
        return false;
    const std::unordered_map<std::string, std::size_t>::const_iterator gone = where.deleted_in.find(name);
    return gone != where.deleted_in.end() && gone->second != 0 &&
           std::find(where.block_numbers.begin(), where.block_numbers.end(), gone->second) ==
               where.block_numbers.end();
}

// A DECLARATION TAKES ITS NAME, OR ANSWERS FALSE WHEN THE NAME IS TAKEN -- the second
// declaration a program is refused for. AT THE PROMPT the name may be one an earlier
// line kept, and declaring it again replaces it, type and all (program_walk.hpp's
// TypedLineMemory says why) -- after the line, so the old type judges its value.
bool declares(Where &where, DeclaredNames &declared, const std::string &name, Code code)
{
    if (deleted_in_another_block(where, name))
        return false;
    if (declared.emplace(name, code).second) {   // written into its place in satellite.legal (LegalNames)
        where.declared_in[name] = where.this_block();
        where.made_by.erase(name);              // the_names_a_line_makes writes it again for a section's line
        where.deleted_in.erase(name);
        where.deleted_on_line.erase(name);
        where.ended_at.erase(name);
        return true;
    }
    if (!where.typed_line)
        return false;
    // A KEPT NAME DECLARED AGAIN IS THE SAME LIVE NAME, and keeps the block it had: declared again inside an if, it
    // does not end at that if's } (the fresh reader, 2026-10-06; the walker's VariableTable::declare agrees).
    where.replacing.emplace_back(name, code);
    return true;
}

// THE LINE A PERSON WROTE `at` ON: a file's line as written; at the prompt, the line of what was typed
// -- a statement typed after the session declared capsules is checked as the body of a hidden capsule
// placed after those declarations (session.cpp's "<prompt>"), on lines nobody typed (the fresh reader,
// 2026-10-06).
std::size_t line_a_person_wrote(const Where &where, const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const std::string file = file_of(where);
    std::size_t line = line_of(row, at), column = 0;
    if (file == "<prompt>" && where.site != nullptr) {
        const std::size_t opened = line_of(row, where.site->body);
        return line > opened ? line - opened : line;
    }
    std::string text;
    written_place(file, row, at, line, column, text);
    return line;
}

// A SECTION'S OR A DISPLAY'S NAME WRITTEN BARE WHERE NO LINE DECLARED IT (SEC-0, rule 4): most often in another
// capsule, which is told how that capsule reaches it -- "a satellite.variable line" alone was wrong advice, since the
// capsule that made it had none either (the fresh reader of SEC-0, 2026-10-25). "" for any other name.
std::string a_name_made_elsewhere(const Where &where, const std::string &name)
{
    if (where.widget_names == nullptr || where.widget_names->count(name) == 0)
        return std::string();
    return " -- a section's or a display's name is a name in the capsule whose line made it; a capsule given the "
           "window reaches it as its_window." + name;
}

// A NAME NO LONGER LEGAL HERE, NAMED AGAIN: one whose block ended is told the } that ended it and its place in
// satellite.legal; one satellite.delete took, the line it went on -- and, when it went inside a block that has
// ended, that only that block may declare it again. Any other name is told what it always was, and a section's or a
// display's how another capsule reaches it.
std::string not_declared_why(const Where &where, const std::vector<std::bitset<16>> &row, const std::string &name)
{
    if (where.ended_at.count(name) == 0 && where.ended_earlier.count(name) != 0)
        return name + " is not legal here -- it was declared inside a block that has ended" +
               (where.place != nullptr ? ", and " + where.place->name + " holds it no further" : std::string()) +
               "; declare it again to use it";
    const std::unordered_map<std::string, std::size_t>::const_iterator ended = where.ended_at.find(name);
    if (ended != where.ended_at.end())
        return name + " is not legal here -- it was declared inside the block that ended at the } on line " +
               std::to_string(line_a_person_wrote(where, row, ended->second)) +
               (where.place != nullptr ? ", and " + where.place->name + " holds it no further" : std::string()) +
               "; declare it above that block to use it after it";
    const std::unordered_map<std::string, std::size_t>::const_iterator gone = where.deleted_on_line.find(name);
    if (gone == where.deleted_on_line.end())
        return name_not_declared_why(name) + a_name_made_elsewhere(where, name);
    return name + " was deleted by satellite.delete(" + name + ") on line " + std::to_string(gone->second) +
           ", and no line after that one can use it -- " +
           (deleted_in_another_block(where, name) ? "it went inside a block, and only that block may declare it again"
                                                  : "declare it again first");
}

// arguments.access DECIDES WHETHER NAMES END AT THEIR BLOCK (satellite_legal.hpp): on with no row, its default.
// Read once, as the checker starts, and handed to the walker in MachineState.
bool names_end_at_their_block(const Arguments *arguments)
{
    return arguments == nullptr || arguments->find("arguments.access") == nullptr ||
           arguments->flag("arguments.access");
}

// A BLOCK'S } (satellite_legal.hpp): while names end at their block, every name declared in it stops being legal
// here -- forgotten as satellite.delete forgets one, with where the } stood for the line that names it later --
// and its place in satellite.legal holds it as ended. A name satellite.delete took inside the block ends with it.
void close_the_block(Where &where, DeclaredNames &declared, std::size_t at)
{
    const std::size_t closing = where.this_block();
    where.closed();
    if (!where.names_end_with_their_block || closing == 0)
        return;
    for (std::unordered_map<std::string, std::size_t>::iterator each = where.declared_in.begin();
         each != where.declared_in.end();) {
        if (each->second != closing) { ++each; continue; }
        declared.erase(each->first);   // ended in its place in satellite.legal, its record kept (LegalNames)
        where.objects.erase(each->first);
        where.lists.erase(each->first);
        where.multiples.erase(each->first);
        where.ended_at[each->first] = at;
        each = where.declared_in.erase(each);
    }
    for (std::unordered_map<std::string, std::size_t>::iterator each = where.deleted_in.begin();
         each != where.deleted_in.end();) {
        if (each->second != closing) { ++each; continue; }
        where.ended_at[each->first] = at;
        where.deleted_on_line.erase(each->first);
        each = where.deleted_in.erase(each);
    }
}

// A NAME THAT HOLDS OBJECTS, OR A LIST OF THEM, FROM ITS DECLARED SHAPE -- kept or
// forgotten, so a name declared again as something else is not judged as the old one.
//
// A `multiple` WITH ONE SPACESUIT AMONG ITS TYPES holds an object of that one whenever
// it holds an object (2026-09-26), so a CAPSULE after its dot is judged by it:
// `multiple<string, shelf> either = s` then `either.call_read("a")`, which was refused as
// "only an object of a satellite.spacesuit has capsules to call". Kept apart from
// `objects`, because its other types keep their methods -- `multiple<list, shelf> e`
// still appends (the review, 2026-09-26: in `objects`, e.append(2) was told "shelf has
// no append"). Two different spacesuits among its types would each need judging, and
// stay refused.
std::size_t suit_it_may_hold(const TypeShape &shape)
{
    if (shape.is_a_suit()) return shape.suit;
    if (shape.word != word::code_of(1, 4, 6)) return kNoSuit;
    std::size_t found = kNoSuit;
    for (const TypeShape &arm : shape.parameters) {
        const std::size_t one = suit_it_may_hold(arm);
        if (one == kNoSuit) continue;
        if (found != kNoSuit && found != one) return kNoSuit;
        found = one;
    }
    return found;
}

void remember_shape(Where &where, const std::string &name, const TypeShape &shape)
{
    where.shapes[name] = shape;
    if (shape.is_a_suit())
        where.objects[name] = shape.suit;
    else
        where.objects.erase(name);
    const std::size_t suit = shape.is_a_suit() ? kNoSuit : suit_it_may_hold(shape);
    if (suit != kNoSuit)
        where.multiples[name] = suit;
    else
        where.multiples.erase(name);
    if (shape.word == word::code_of(1, 4, 2) && shape.parameters.size() == 1 && shape.parameters[0].is_a_suit())
        where.lists[name] = shape.parameters[0].suit;
    else
        where.lists.erase(name);
}

// A for's NUMBER OUTLIVES NOTHING (MILESTONES M20.A: it "exists while the for
// loop is running"), so the checker has to forget it where the walker erases it,
// or `i` written after the loop would pass the check and be refused at run time --
// with the loop's own output already printed, which is the one thing this whole
// file exists to prevent. Each entry is the code just past a for's `}` and the
// name that dies there; forget_the_finished below is called before every
// statement, so a name is gone by the first statement at or after that point.
//
// It is a list and not a stack because it is read by POSITION: nested loops end
// in the order they must, and two loops that use `i` one after the other are both
// allowed -- the first has forgotten it before the second declares it.
using EndingNames = std::vector<std::pair<std::size_t, std::string>>;

void forget_the_finished(EndingNames &ending, std::size_t at, DeclaredNames &declared)
{
    for (std::size_t which = ending.size(); which > 0; --which) {
        if (ending[which - 1].first > at)
            continue;
        declared.erase(ending[which - 1].second);
        ending.erase(ending.begin() + static_cast<std::ptrdiff_t>(which - 1));
    }
}

// satellite.delete(name), JUDGED BEFORE ANYTHING RUNS (delete_calls.hpp, NEW_MILESTONES.md NM-2): a
// variable this body declared, in the block this line stands in -- and then FORGOTTEN from here on,
// its name, its type and the three lists of names that hold objects, as the walker erases it, so the
// next line that names it is refused before anything runs and the name may be declared again.
signed long long int judge_a_delete(Where &where, DeclaredNames &declared, const EndingNames &ending,
                                    const std::vector<std::bitset<16>> &row, const std::string &name,
                                    std::string &why)
{
    // `arguments` IS A NAME LIKE ANY OTHER SINCE 2026-10-11 (main_arguments.hpp): the run's own are
    // satellite.arguments, which is no name and is not deleted -- satellite.delete takes one name.
    if (declared.find(name) == declared.end()) {
        why = not_declared_why(where, row, name);
        return name_not_declared;
    }
    // A FIELD: a spacesuit's capsule has its object's fields as its first declared names (check_program).
    if (where.site != nullptr && where.site->suit != kNoScope) {
        const CapsuleScope &suit = where.capsules.scopes[where.site->suit];
        if (suit.layout != nullptr && suit.layout->slot_of(name) != kNoSlot) {
            why = name + " is a field of " + suit.layout->shown + ", which belongs to the object and goes when the "
                  "object goes -- satellite.delete takes a variable of this capsule's own";
            return satl_line_not_understood;
        }
    }
    for (const std::pair<std::size_t, std::string> &dies : ending)
        if (dies.second == name) {
            why = name + " is satellite.statement.for's own number, and it ends with its loop -- it is not deleted "
                  "inside it";
            return satl_line_not_understood;
        }
    // THE SAME BLOCK. The checker reads a body once, top to bottom: a delete in another block would
    // leave every later line depending on whether that block ran -- and in a loop, its next pass
    // finding the name already gone.
    const std::unordered_map<std::string, std::size_t>::const_iterator in = where.declared_in.find(name);
    // A PARAMETER OF A CAPSULE WITH A satellite.error() (ERROR_BLOCK_PLAN.md), whose body is judged as a block: the
    // error block sees the parameters whichever line the error came on, so none is deleted (the fresh reader).
    if (in == where.declared_in.end() && where.site != nullptr && where.site->error_body != kNoErrorBlock) {
        why = "satellite.delete(" + name + ") deletes a name " + where.site->shown + " was given, and " +
              where.site->shown + " has a satellite.error() block, which sees what it was given whichever line the "
              "error came on -- so it keeps " + name + " to its end";
        return satl_line_not_understood;
    }
    if ((in == where.declared_in.end() ? 0 : in->second) != where.this_block()) {
        why = "satellite.delete(" + name + ") stands in a different block from the line that declared " + name +
              " -- whether " + name + " is there to delete, and whether the lines after it may still use it, "
              "would depend on which blocks ran; delete it in the block that declared it";
        return satl_line_not_understood;
    }
    declared.erase(name);
    where.declared_in.erase(name);
    where.objects.erase(name);
    where.lists.erase(name);
    where.multiples.erase(name);
    where.deleted_in[name] = where.this_block();
    where.deleted_on_line[name] = line_a_person_wrote(where, row, where.statement);
    return success;
}

// A BINARY IS WRITTEN WITH ITS b (the author, 2026-09-16): "if the user doesn't
// enter "b" and enters satellite.variable.binary just require them to enter the
// b, spit out an ERROR: expected "b"+whatever they entered".
//
// `at` is the first code of the value, straight after the `=`. A bare number
// there is the mistake, and the answer is what they wrote with a b in front:
//
//     satellite.variable.binary my_number = 10101010
//     ERROR: expected b10101010
//
// THE SUGGESTION IS NOT QUOTED, though the author's sentence quotes the b. In
// satellite a quote makes a STRING, so `expected "b10101010"` would point at the
// one spelling that is still wrong. If the author wants the quotes, it is this
// one string.
//
// ONLY THE FIRST VALUE IS JUDGED, which is the author's case -- the value they
// entered. `bits = b1010 * 2` is not this mistake (the 2 is a count, not bits)
// and is left to the walker, which refuses the number the arithmetic answers.
//
// DIGITS THAT ARE NOT ALL 0 AND 1 GET THEIR OWN SENTENCE, because `expected b12`
// would send a person to write b12, which is not binary either. The same goes
// for `b12` itself: the lexer makes a NAME of it (b and 0s and 1s is the only
// binary it knows), and "b12 has no satellite.variable line" is true and useless.
signed long long int binary_is_written_with_b(const std::vector<std::bitset<16>> &row, std::size_t at,
                                              const DeclaredNames &declared, std::string &why)
{
    // A BRACKET IS NOT A VALUE, so `= (10101010)` is judged by what is inside it.
    // Without this the brackets hid the mistake and the program ran first.
    //
    // A MINUS SIGN IS PART OF THE VALUE, as it is for a percentage: a binary keeps
    // its sign (the author, 2026-09-17: "give it a different number and keep a
    // sign"), so -b1010 declares, and `= -1010` is ERROR: expected -b1010. Each
    // minus turns the suggestion over, as it would the value. (For one commit the
    // minus was refused outright -- "a binary has no minus sign" -- which is what
    // that ruling answered.)
    bool below_zero = false;
    for (;; ++at) {
        const Code ahead = code_at(row, at);
        if (ahead == token::tight_minus_token || ahead == token::minus_token)
            below_zero = !below_zero;
        else if (ahead != token::left_parenthesis_token)
            break;
    }
    const std::string sign = below_zero ? "-" : "";
    const Code code = code_at(row, at);
    if (code != token::number_token && code != token::name_token)
        return success;
    std::size_t k = at;
    const std::string entered = text_at(row, k);

    // 0b10101010 AND 0x1F, the C and Python spellings. The lexer reads the 0 as a
    // number and the rest as a b or x literal of its own, so without this the
    // answer was `expected b0` -- a real binary, and the wrong one.
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::binary_token) {
        std::size_t digits = k;
        why = "ERROR: expected " + sign + "b" + text_at(row, digits);
        return types_do_not_meet;
    }
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::hexadecimal_token) {
        std::size_t digits = k;
        why = "ERROR: " + sign + "0x" + text_at(row, digits) + " is not binary -- binary is b and then 0s and 1s, like b1010";
        return types_do_not_meet;
    }

    bool only_bits = true;
    for (const char c : entered) only_bits = only_bits && (c == '0' || c == '1');

    if (code == token::number_token) {
        why = only_bits ? "ERROR: expected " + sign + "b" + entered
                        : "ERROR: " + sign + entered + " is not binary -- binary is b and then 0s and 1s, like b1010";
        return types_do_not_meet;
    }

    // A NAME: only `b` and then digits, and only when nothing declared it.
    if (declared.find(entered) != declared.end() || entered.size() < 2 || entered[0] != 'b')
        return success;
    for (std::size_t i = 1; i < entered.size(); ++i)
        if (entered[i] < '0' || entered[i] > '9')
            return success;
    why = "ERROR: " + sign + entered + " is not binary -- a binary digit is 0 or 1";
    return types_do_not_meet;
}

// A PERCENTAGE IS WRITTEN WITH ITS %, by the same rule as a binary's b (the
// author's for binary, 2026-09-16, applied here on 2026-09-17 because a
// percentage has the same shape of mistake): `satellite.variable.percentage p = 50`
// is ERROR: expected 50%. The % is in the text, so nothing has to run to see it
// is missing. Only the first value, inside any brackets, as for binary.
//
// A MINUS SIGN IS PART OF THE VALUE, so `p = -50` is ERROR: expected -50% (the
// review of ab01a74, 2026-09-17: the - was not skipped, so the program printed
// first and was refused at run time). -50% is a real percentage, and the
// suggestion keeps the sign: each minus turns it over, as it would the value.
signed long long int percentage_is_written_with_percent(const std::vector<std::bitset<16>> &row, std::size_t at,
                                                        std::string &why)
{
    bool below_zero = false;
    for (;; ++at) {
        const Code code = code_at(row, at);
        if (code == token::tight_minus_token || code == token::minus_token)
            below_zero = !below_zero;
        else if (code != token::left_parenthesis_token)
            break;
    }
    if (code_at(row, at) != token::number_token)
        return success;
    std::size_t k = at;
    why = "ERROR: expected " + std::string(below_zero ? "-" : "") + text_at(row, k) + "%";
    return types_do_not_meet;
}

// A TIME IS WRITTEN WITH ITS UNIT, by the percentage's rule above (the author, 2026-10-10: "we need
// to accept s or ms or ns as a time"): `satellite.variable.time t = 5` is ERROR: expected 5s, 5ms
// or 5ns -- a bare number names no unit, and the three it could be differ a billion times over. A
// minus sign is kept in the suggestion, as the percentage keeps it.
signed long long int time_is_written_with_its_unit(const std::vector<std::bitset<16>> &row, std::size_t at,
                                                   std::string &why)
{
    bool below_zero = false;
    for (;; ++at) {
        const Code code = code_at(row, at);
        if (code == token::tight_minus_token || code == token::minus_token)
            below_zero = !below_zero;
        else if (code != token::left_parenthesis_token)
            break;
    }
    if (code_at(row, at) != token::number_token)
        return success;
    std::size_t k = at;
    const std::string shown = std::string(below_zero ? "-" : "") + text_at(row, k);
    why = "ERROR: expected " + shown + "s, " + shown + "ms or " + shown + "ns";
    return types_do_not_meet;
}

// AFTER A DECLARED NAME, ONLY `=` -- or the line's end, for a declaration with no
// value yet. `n += 1` was a line the walker skipped without a word: n kept its
// value and the program exited 0, because run_assignment read anything that was
// not `=` as "a declaration with no value" (found by the binary review,
// 2026-09-16, and true of every type). `+=` and its family are real tokens with
// no scenario, so they are not_built_yet by name; anything else is not a
// statement a name can start. `k` is the code straight after the name.
signed long long int after_the_name(const std::vector<std::bitset<16>> &row, std::size_t k,
                                    const std::string &name, bool declaring, std::string &why)
{
    const Code code = code_at(row, k);
    if (code == token::assign_token)
        return success;
    const bool ends = code == token::line_end_token || code == token::comment_token ||
                      code == token::end_of_file_token;
    if (ends && declaring)
        return success;

    const char *sign = code == token::plus_assign_token ? "+"
                     : code == token::minus_assign_token ? "-"
                     : code == token::times_assign_token ? "*"
                     : code == token::divide_assign_token ? "/"
                     : code == token::modulus_assign_token ? "%" : nullptr;
    if (sign != nullptr) {
        why = name + " " + sign + "= ... is not built yet -- write " + name + " = " + name + " " + sign + " ...";
        return not_built_yet;
    }
    // A DECLARED NAME RUNNING INTO SOMETHING is most often a name with a character a name
    // cannot hold -- `poly.#@($` stops at the dot, `café` at the é -- so the rule is said
    // with it (M5, DESIGN §7's own entries).
    why = ends       ? name + " on its own line does nothing -- give it a value with ="
          : declaring ? name + " is followed by something that is not = -- if that is part of the name, " + kNameRule
                      : name + " is followed by something that is not = , and there is no statement of that shape";
    return satl_line_not_understood;
}

// THE BRACKETS THAT OPEN AT `open` -- a `(` or a `[` -- and how many arguments
// they hold: 0 for empty ones, otherwise the commas at their own depth plus one.
// `close` is left on the bracket that closes them. False when they never close on
// this line. A payload is skipped, never read, so a comma or a bracket inside a
// string is not one of theirs.
bool brackets_at(const std::vector<std::bitset<16>> &row, std::size_t open, std::size_t &close, std::size_t &count);

// ONE SIZE OF A SECTION, AT `at`, WHEN IT IS A LITERAL ALONE -- 50%, 1/2, 400, a minus before one of them, touching
// or spaced, as the run's own unary minus takes both (expression.cpp); "wide", 2.5, a hex, a binary, a time,
// satellite.bool.true -- made into the value the run would make (expression.cpp's literals) and judged as the run
// judges it (SEC-1). `at` is left on the comma or the bracket after the argument either way, so the next one can
// be judged. Anything else is an expression the run works out, and passes here.
signed long long int size_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t &at,
                                          const std::string &what, std::string &why)
{
    std::size_t k = at;
    const bool below_zero = code_at(row, k) == token::tight_minus_token || code_at(row, k) == token::minus_token;
    if (below_zero)
        ++k;
    const Code code = code_at(row, k);
    Value made;
    bool a_literal = true;
    if (code == token::percentage_token) {
        satellite_percentage percent;
        a_literal = satellite_percentage::from_digits(text_at(row, k), percent) == success;
        if (below_zero)
            percent.scaled = -percent.scaled;
        made = Value::of_percentage(std::move(percent));
    } else if (code == token::number_token) {
        const std::string digits = text_at(row, k);
        std::string unused;
        if (code_at(row, k) == token::fraction_token) {
            a_literal = fraction_literal(digits, row, k, made, unused) == success;
            if (a_literal && below_zero)
                made = Value::of_fraction(made.as_fraction()->negated());
        } else if (digits.find('.') != std::string::npos) {
            a_literal = float_literal(digits, made, unused) == success;
        } else {
            satellite_number number;
            a_literal = number_fast_path::from_token_text(digits, 10, number) == success;
            made = Value::of_number(below_zero ? -number : number);
        }
    } else if (code == token::string_token && !below_zero) {
        satellite_string text;
        std::size_t bad_character = 0;
        a_literal = string_literal_at(row, k, text, bad_character) == success;
        made = Value::of_string(std::move(text));
    } else if (code == token::hexadecimal_token && !below_zero) {
        std::string unused;
        a_literal = hexadecimal_literal(text_at(row, k), made, unused) == success;
    } else if (code == token::binary_token && !below_zero) {
        satellite_binary_number bits;
        a_literal = satellite_binary_number::from_digits(text_at(row, k), bits) == success;
        made = Value::of_binary(std::move(bits));
    } else if (code == token::time_token && !below_zero) {
        const std::string written = text_at(row, k);
        satellite_time::Unit unit = satellite_time::Unit::second;
        std::size_t suffix = 0;
        satellite_time length;
        a_literal = satellite_time::unit_at_the_end(written, unit, suffix) &&
                    satellite_time::from_digits(written.substr(0, written.size() - suffix), unit, length) == success;
        made = Value::of_time(std::move(length));
    } else if ((code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) && !below_zero) {
        made = Value::of_bool(code == word::code_of(1, 17, 2));
        ++k;
    } else {
        a_literal = false;
    }
    // ALONE: the comma or the bracket straight after it. `1 / 3` and `50% / 2` go on past it, and are the run's.
    a_literal = a_literal && (code_at(row, k) == token::comma_token || code_at(row, k) == token::right_parenthesis_token);
    std::size_t depth = 0;
    while (k < row.size() && !(depth == 0 && (code_at(row, k) == token::comma_token ||
                                              code_at(row, k) == token::right_parenthesis_token))) {
        const Code next = code_at(row, k);
        if (next == token::line_end_token || next == token::end_of_file_token)
            break;
        if (token::carries_a_count(next)) { text_at(row, k); continue; }
        if (next == token::left_parenthesis_token || next == token::left_square_bracket_token ||
            next == token::left_brace_token)
            ++depth;
        else if ((next == token::right_parenthesis_token || next == token::right_square_bracket_token ||
                  next == token::right_brace_token) && depth > 0)
            --depth;
        ++k;
    }
    at = k;
    if (!a_literal)
        return success;
    AnExtent unused;
    return extent_refused(made, unused, what, why);
}

} // namespace

// THE ONE window_calls.hpp DECLARES, outside the anonymous namespace since SEC-0: check_program's pass over every name
// a program gives a widget asks it too, wherever `.satellite.window.section` is written.
signed long long int section_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t open,
                                          const std::string &spelling, std::string &why)
{
    std::size_t close = open, given = 0;
    if (code_at(row, open) != token::left_parenthesis_token || !brackets_at(row, open, close, given)) {
        why = spelling + " takes a name, a width and a height, in brackets: " + spelling + "(\"top\", 100%, 50%)";
        return satl_line_not_understood;
    }
    if (given != 3) {
        why = spelling + " takes 3 arguments -- a name, a width and a height -- and was given " + std::to_string(given);
        return satl_line_not_understood;
    }
    std::size_t k = open + 1;
    if (code_at(row, k) != token::string_token) {
        why = spelling + " takes the section's name, in quotes, first: " + spelling + "(\"top\", 100%, 50%) -- a "
              "name worked out by the run could not be checked before it";
        return satl_line_not_understood;
    }
    const std::string named = text_at(row, k);
    if (!a_widget_name_fits(named, why))
        return satl_line_not_understood;
    // ITS WIDTH AND ITS HEIGHT, WHEN EACH IS WRITTEN AS A LITERAL (SATELLITE_WINDOW_SECTIONS.md SEC-1): refused before
    // anything runs, in the sentence the run would say (window_readers.cpp's extent_refused). A size worked out by the
    // run -- a name, `100% / 3`, `1 / 3` -- is the run's to judge.
    for (const char *which : {"'s width", "'s height"}) {
        if (code_at(row, k) != token::comma_token)
            return success;
        ++k;
        const signed long long int judged = size_literal_refused(row, k, named + which, why);
        if (judged != success)
            return judged;
    }
    return success;
}

namespace {

// THE BRACES THAT OPEN AT `open`, a list or a map written whole where a value goes: `close` on the `}` that closes
// them, and `a_map` when a `:` follows their first item at their own depth, which is how the walker tells a map
// (expression.cpp's map_literal). False when they never close on this line. A payload is skipped, never read.
bool braces_at(const std::vector<std::bitset<16>> &row, std::size_t open, std::size_t &close, bool &a_map)
{
    std::size_t depth = 0, brackets = 0;
    bool past_the_first = false;
    a_map = false;
    std::size_t at = open;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        if (code == token::line_end_token || code == token::end_of_file_token) break;
        if (code == token::left_brace_token) {
            ++depth;
        } else if (code == token::right_brace_token) {
            if (--depth == 0) { close = at; return true; }
        } else if (code == token::left_parenthesis_token || code == token::left_square_bracket_token) {
            ++brackets;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token) {
            if (brackets > 0) --brackets;
        } else if (depth == 1 && brackets == 0) {
            if (code == token::colon_token && !past_the_first) a_map = true;
            if (code == token::comma_token) past_the_first = true;
        }
        ++at;
    }
    close = at;
    return false;
}

// A FILE'S KIND WRITTEN AS A LITERAL IS JUDGED BEFORE THE RUN (the checker gaps of 2026-10-19, OPEN_ITEMS_2026-10-20):
// satellite.file.open(path, "append") or new(path, "list") is refused here, in the run's own sentence (file_calls.cpp's
// kind_of), and not after the lines above it have run. The second argument is found as brackets_at counts them -- a
// payload skipped, a comma inside a string, a bracket or a braced list not counted -- and only a string literal
// standing alone there is judged; a kind worked out from a name is still the walker's to judge.
signed long long int file_kind_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t open,
                                             const std::string &spelling, std::string &why)
{
    std::size_t depth = 0, braces = 0, at = open;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        if (code == token::line_end_token || code == token::end_of_file_token) return success;
        if (code == token::left_brace_token) {
            ++braces;
        } else if (code == token::right_brace_token) {
            if (braces > 0) --braces;
        } else if (code == token::left_parenthesis_token || code == token::left_square_bracket_token) {
            ++depth;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token) {
            if (--depth == 0) return success;
        } else if (code == token::comma_token && depth == 1 && braces == 0) {
            std::size_t k = at + 1;
            if (code_at(row, k) != token::string_token) return success;
            const std::string kind = string_at(row, k);
            if (code_at(row, k) != token::right_parenthesis_token || kind == "text" || kind == "binary")
                return success;
            why = spelling + "'s second argument is \"text\" or \"binary\", and it was given \"" + kind + "\"";
            return types_do_not_meet;
        }
        ++at;
    }
    return success;
}

bool brackets_at(const std::vector<std::bitset<16>> &row, std::size_t open, std::size_t &close, std::size_t &count)
{
    std::size_t depth = 0, commas = 0;
    // A BRACED LIST IS ONE ARGUMENT, HOWEVER MANY COMMAS IT HOLDS. Counted apart
    // from `depth` rather than folded into it, because a `}` must never be able
    // to close the brackets: depth reaching 0 is what ends this loop, and a
    // stray brace driving it there would make `f({a, b}` look closed.
    //
    // Without this, `satellite.feedback({"a", "b"})` is refused before it runs,
    // by a checker counting two arguments in a call that takes one -- which is
    // exactly the message a person would get for their own mistake, so it must
    // not be given for the language's.
    std::size_t braces = 0;
    bool any = false;
    std::size_t at = open;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { any = true; text_at(row, at); continue; }
        if (code == token::line_end_token || code == token::end_of_file_token) break;
        if (code == token::left_brace_token) {
            any = true;
            ++braces;
        } else if (code == token::right_brace_token) {
            any = true;
            if (braces > 0) --braces;
        } else if (code == token::left_parenthesis_token || code == token::left_square_bracket_token) {
            if (depth > 0) any = true;
            ++depth;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token) {
            if (--depth == 0) { close = at; count = any ? commas + 1 : 0; return true; }
        } else {
            any = true;
            if (code == token::comma_token && depth == 1 && braces == 0) ++commas;
        }
        ++at;
    }
    close = at;
    count = 0;
    return false;
}

// THE FOUR TYPES OF 2026-09-22 JUDGE THEIR OWN VALUES AND THEIR OWN METHODS, each
// in bytecode/<name>_values.cpp, so the four could be built side by side without
// every one of them editing this file. `type` is the declared word; any other
// word is none of theirs.
bool one_of_the_four(Code type)
{
    return type == word::code_of(1, 6, 10) || type == word::code_of(1, 6, 11) ||
           type == word::code_of(1, 6, 19) || type == word::code_of(1, 6, 20);
}

signed long long int written_right_for(Code type, const std::vector<std::bitset<16>> &row, std::size_t at,
                                       const DeclaredNames &declared, std::string &why)
{
    if (type == word::code_of(1, 6, 10)) return float_is_written_right(row, at, declared, why);
    if (type == word::code_of(1, 6, 11)) return hexadecimal_is_written_right(row, at, declared, why);
    if (type == word::code_of(1, 6, 19)) return color_is_written_right(row, at, declared, why);
    return success;
}

// A LONE LITERAL GIVEN TO A NAME OF A PLAIN TYPE IS JUDGED BEFORE ANYTHING RUNS (ERRORS2 #8,
// 2026-09-26). `satellite.variable.number n = "abc"` was refused only when its line ran, so the
// lines above it printed first -- while a literal's kind is known from its token alone. The
// table is what the walker takes, measured cell by cell (a number takes 5, b101 and x1F; a
// float takes 5 and 1.5; a fraction 5 and 1/3; an infinity 5), and the sentence is the
// walker's, so the refusal reads the same, only sooner. A value worked out -- a name, a call,
// a sum -- is still judged when it runs, and so is a colour, whose literals are its own
// (color_check.cpp).
enum class Literal { none, text, whole, decimal, boolean, binary, percentage, hexadecimal, fraction, time };

Literal lone_literal(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    std::size_t k = at;
    const Code code = code_at(row, k);
    Literal kind = Literal::none;
    if (code == token::string_token) {
        string_at(row, k);
        kind = Literal::text;
    } else if (code == token::number_token) {
        const std::string digits = text_at(row, k);
        kind = digits.find('.') == std::string::npos ? Literal::whole : Literal::decimal;
        if (code_at(row, k) == token::fraction_token) {
            ++k;
            if (code_at(row, k) != token::number_token) return Literal::none;
            text_at(row, k);
            kind = Literal::fraction;
        }
    } else if (code == token::binary_token || code == token::hexadecimal_token || code == token::percentage_token) {
        text_at(row, k);
        kind = code == token::binary_token ? Literal::binary
             : code == token::hexadecimal_token ? Literal::hexadecimal : Literal::percentage;
    } else if (code == token::time_token) {        // 1s, 250ms, 30ns (satellite_time.hpp)
        text_at(row, k);
        kind = Literal::time;
    } else if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) {   // satellite.bool.false, .true
        ++k;
        kind = Literal::boolean;
    }
    const Code after = code_at(row, k);
    const bool ends = after == token::line_end_token || after == token::comment_token || after == token::end_of_file_token;
    return ends ? kind : Literal::none;
}

// "" when a name declared `type` takes this literal, or what it holds, as kind_name says it.
std::string literal_refused(Code type, Literal kind)
{
    if (kind == Literal::none) return "";
    const auto is = [&](std::initializer_list<Literal> taken) {
        for (const Literal each : taken) if (each == kind) return true;
        return false;
    };
    bool takes = true;
    if (type == word::code_of(1, 6, 4)) takes = is({Literal::whole, Literal::binary, Literal::hexadecimal});   // number
    else if (type == word::code_of(1, 6, 1)) takes = is({Literal::text});                                    // string
    else if (type == word::code_of(1, 6, 6)) takes = is({Literal::boolean});                                 // bool
    else if (type == word::code_of(1, 6, 10)) takes = is({Literal::whole, Literal::decimal});                // float
    else if (type == word::code_of(1, 6, 5)) takes = is({Literal::binary});                                  // binary
    else if (type == word::code_of(1, 6, 16)) takes = is({Literal::percentage});                             // percentage
    else if (type == word::code_of(1, 6, 11)) takes = is({Literal::hexadecimal});                            // hex
    else if (type == word::code_of(1, 6, 20)) takes = is({Literal::whole, Literal::fraction});               // fraction
    else if (type == word::code_of(1, 6, 17)) takes = false;      // infinity: a minimum of infinity, never 5 (his H)
    else if (type == word::code_of(1, 6, 2)) takes = false;                                                  // file
    else if (type == word::code_of(1, 6, 23)) takes = is({Literal::text});                                  // program
    else if (type == word::code_of(1, 6, 24)) takes = is({Literal::text});                                  // bash
    else if (type == word::code_of(1, 6, 3)) takes = is({Literal::time});                                   // time
    if (takes) return "";
    switch (kind) {
    case Literal::text: return "a string";
    case Literal::whole: return "a number";
    case Literal::decimal: return "a float";
    case Literal::boolean: return "a bool";
    case Literal::binary: return "a binary";
    case Literal::percentage: return "a percentage";
    case Literal::hexadecimal: return "a hex";
    case Literal::fraction: return "a fraction";
    case Literal::time: return "a time";
    case Literal::none: break;
    }
    return "";
}

// `name = <literal>` or a declaration's: refused in the walker's own sentence, or success.
signed long long int literal_fits_the_name(Code type, const std::string &name, const std::vector<std::bitset<16>> &row,
                                           std::size_t value_at, std::string &why)
{
    const Literal kind = lone_literal(row, value_at);
    // A FRACTION GIVEN TO A NUMBER OR A FLOAT NAME is fraction_given_where_none_goes's, asked next, which
    // says what to write (his 9b of 2026-10-12) -- the same sentence for 5/4 as for 5 / 4.
    if (kind == Literal::fraction && (type == word::code_of(1, 6, 4) || type == word::code_of(1, 6, 10)))
        return success;
    const std::string holds = literal_refused(type, kind);
    if (holds.empty()) return success;
    why = name + " was declared " + std::string(word::spelling_of(type)) + ", and it holds " + holds;
    // AN INFINITY NAME HOLDS AN INFINITY (the author's H of 2026-10-11: "= 5 is an error, the correct syntax should
    // be a minimum of infinity"), and the sentence says what the least of one is -- in type_shape.cpp's words.
    if (type == word::code_of(1, 6, 17)) why += kAMinimumOfInfinity;
    return types_do_not_meet;
}

signed long long int method_right_for(Code type, Code method, const std::string &spelling, std::string &why)
{
    if (type == word::code_of(1, 6, 10)) return float_method_check(method, spelling, why);
    if (type == word::code_of(1, 6, 11)) return hexadecimal_method_check(method, spelling, why);
    if (type == word::code_of(1, 6, 19)) return color_method_check(method, spelling, why);
    return fraction_method_check(method, spelling, why);
}

// A METHOD ON A DECLARED NAME, judged by the name's declared TYPE -- which the
// checker has, since DeclaredNames keeps the declaring word (the review,
// 2026-09-18: `n.append("x")` on a number and `f.replace(1)` on a file passed the
// check and were refused after earlier lines had printed). `k` is the code after
// the name. Only the first method is judged: what a method answers is a run-time
// fact, so a chain's later segments are left to the walker. `spelling` is the name and
// the method as the refusal says them (method_on_a_name, below).
signed long long int method_judged(const std::vector<std::bitset<16>> &row, std::size_t k,
                                   const std::string &name, Code declared_as, std::string &why,
                                   const std::string &spelling)
{
    const Code method = code_at(row, k + 1);
    const int arity = file_method_arity(method);
    const bool of_a_string_or_number = method == token::find_token || method == token::add_token ||
                                       method == token::to_string_token || method == token::to_number_token ||
                                       method == token::to_binary_token || method == token::to_hexadecimal_token;

    // A CONTAINER'S OWN METHODS (the author, 2026-09-18). `.reverse()` is not in
    // this list because it is not a container's: it is on every type that has an
    // order -- a string, a number, a binary -- so it is allowed on anything here
    // and refused at the value, where the kind is actually known.
    // ASKED, NEVER COPIED: container_arity IS the list of container methods, and
    // it lives beside the code that implements them (container_calls.hpp). The
    // hand-written set that used to be here went stale the same afternoon it was
    // written, refusing `n.first` before the program ran while the walker had it.
    const bool of_a_container = container_arity(method) >= 0;
    const bool a_container = declared_as == word::code_of(1, 4, 2) || is_an_index_word(declared_as) ||
                             declared_as == word::code_of(1, 4, 6) ||
                             declared_as == word::code_of(1, 6, 21) ||   // the arguments: an index
                             declared_as == word::code_of(1, 6, 22);     // an info: a list of indexes

    // .reverse() TAKES NOTHING, on every type that has one (ERRORS4 023): s.reverse("x") and n.reverse(99) ran with
    // what they were given dropped, where s.upper(1) is refused in this sentence (below).
    if (method == token::reverse_token) {
        std::size_t close = k + 2, given = 0;
        if (code_at(row, k + 2) == token::left_parenthesis_token && (!brackets_at(row, k + 2, close, given) || given != 0)) {
            why = spelling + "() takes nothing in its brackets";
            return satl_line_not_understood;
        }
        return success;                  // every type with an order has one
    }
    // object.satellite.pointer() AND .satellite.reference() ARE AN OBJECT'S (pointer_calls.hpp) -- an object
    // of a satellite.spacesuit, the one kind `b = a` shares -- and a name declared any other type has
    // neither. Said as that, and not as a method not built yet; method_on_a_name asks it first, with the
    // new spelling in `spelling`.
    if (method == token::pointer_token || method == token::reference_token) {
        if (declared_as == word::code_of(1, 4, 6))
            return success;              // a multiple may hold an object: the run knows which it holds
        why = spelling + "() is an object's -- an object of a satellite.spacesuit -- and " + name + " is " +
              word::spelling_of(declared_as);
        return satl_line_not_understood;
    }
    if (one_of_the_four(declared_as))
        return method_right_for(declared_as, method, spelling, why);
    // AN INFINITY'S OWN METHODS, numbered at INF-1 and built from INF-4, each named
    // with the milestone that builds it -- before anything runs.
    if (declared_as == word::code_of(1, 6, 17)) {
        const std::string missing = infinity_method_not_built(method);
        if (!missing.empty()) {
            why = spelling + " " + missing;
            return not_built_yet;
        }
    }
    // A NUMBER'S OWN WORDS (number_calls.hpp, NM-8 2026-10-11): how many each takes, and .sqrt as not built,
    // judged here in the walker's sentences so nothing above them prints first. A NAME DECLARED AN INFINITY may
    // hold a plain number (SATELLITE_INFINITY.md Q24), so it has the number's words as well as its own
    // (method_periods.hpp) -- judged here too; its own three that are not built were refused just above.
    // .find IS NO NUMBER'S: it looks for text in a string, and on a number or an infinity it was always refused
    // when its line ran (S301) -- now before anything runs, with the same machine code.
    if (declared_as == word::code_of(1, 6, 4) || declared_as == word::code_of(1, 6, 17)) {
        if (number_a_method_runs(kind_of_type_word(declared_as), method) != 0) {
            std::size_t close = k + 2, given = 0;
            const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
            if (bracketed && !brackets_at(row, k + 2, close, given)) {
                why = spelling + "( is never closed on its line";
                return satl_line_not_understood;
            }
            return number_method_check(bracketed, given, method, spelling, why);
        }
        if (method == token::find_token) {
            why = spelling + " is a string's -- " + word::spelling_of(declared_as) + " has no text to look in";
            return types_do_not_meet;
        }
    }
    // A THREAD'S OWN METHODS (2026-09-23), asked of thread_calls.hpp: start, stop, join and
    // wait, each taking nothing. Before the containers: a list has a .join of its own.
    if (declared_as == word::code_of(1, 6, 13)) {
        if (thread_method_arity(method) < 0) {
            why = spelling + " -- " + thread_methods_are();
            return types_do_not_meet;
        }
        std::size_t close = k + 2, given = 0;
        if (code_at(row, k + 2) != token::left_parenthesis_token || !brackets_at(row, k + 2, close, given) ||
            given != 0) {
            why = spelling + "() takes nothing, in its brackets";
            return satl_line_not_understood;
        }
        return success;
    }
    // A PROGRAM'S OWN METHODS (2026-10-01), asked of program_calls.hpp: start, which takes nothing or
    // "hide"; pass, which takes one thing; and ok, error, join, code, return, end, exit, quit and
    // shutdown, which take nothing. A bash line's are the same (STEP 5). Before the containers, for
    // the same reason: a list has a .join of its own.
    if (is_program_type(declared_as)) {
        const int most = program_method_arity(method);
        if (most < 0) {
            why = spelling + " -- " + program_methods_are(is_bash_type(declared_as));
            return types_do_not_meet;
        }
        std::size_t close = k + 2, given = 0;
        if (code_at(row, k + 2) != token::left_parenthesis_token || !brackets_at(row, k + 2, close, given) ||
            given > static_cast<std::size_t>(most) || given < static_cast<std::size_t>(program_method_least(method))) {
            why = spelling + program_method_takes(method);
            return satl_line_not_understood;
        }
        // start() GIVEN ITS WORD IN QUOTES is judged here, as .sort's is, in the walker's sentence: a word
        // that is not "hide" -- start("hdie") -- is refused before anything runs, not after the programs
        // above it have run (the review of steps 2-5). A word worked out by the run waits for the run.
        if (method == token::start_token && given == 1 && code_at(row, k + 3) == token::string_token) {
            std::size_t at = k + 3;
            const std::string written = text_at(row, at);
            if (code_at(row, at) == token::right_parenthesis_token && written != "hide") {
                why = spelling + "(" + written + ") -- .satellite.start() takes nothing, or \"hide\" to run it with "
                                                 "its output thrown away";
                return satl_line_not_understood;
            }
        }
        return success;
    }
    if (a_container) {
        if (of_a_container) {
            // HOW MANY IT WAS GIVEN, judged here as a file's and a window's are (the
            // review, 2026-09-23): `a.reserve()` and `a.sum(1)` printed whatever came
            // before them and then stopped. Brackets are needed only by a method that
            // takes something -- `a.size` and `a.size()` are one read.
            const int wanted = container_arity(method);
            const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
            std::size_t close = k + 2, given = 0;
            if (bracketed && !brackets_at(row, k + 2, close, given)) {
                why = spelling + "( is never closed on its line";
                return satl_line_not_understood;
            }
            if (!container_given_fits(method, given) || (wanted > 0 && !bracketed)) {
                why = spelling + container_takes(method, wanted) +
                      (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
                return satl_line_not_understood;
            }
            // `.sort` GIVEN ITS WORD IN QUOTES is judged here, in the walker's sentence
            // (container_calls.hpp): "key" or "value" on a name declared a map, and no
            // word at all on one declared a list. A word worked out -- a name, a call --
            // and a name declared a multiple, which may hold either, wait for the run.
            if (method == token::sort_token && given == 1 && code_at(row, k + 3) == token::string_token) {
                const bool a_map = is_an_index_word(declared_as) || declared_as == word::code_of(1, 6, 21);
                const bool a_list = declared_as == word::code_of(1, 4, 2) || declared_as == word::code_of(1, 6, 22);
                std::size_t at = k + 3;
                const std::string written = text_at(row, at);
                const std::string refused = code_at(row, at) == token::right_parenthesis_token && (a_map || a_list)
                                                ? sort_word_refused(written, a_map, name)
                                                : std::string();
                if (!refused.empty()) {
                    why = refused;
                    return a_map ? satl_line_not_understood : types_do_not_meet;
                }
            }
            // A NAME DECLARED AN INDEX is told which half to ask, before the run, in
            // the walker's own sentence (container_calls.hpp).
            if (is_an_index_word(declared_as)) {
                const std::string refused = index_refuses(method, name);
                if (!refused.empty()) {
                    why = spelling + " -- " + refused;
                    return types_do_not_meet;
                }
            }
            return success;
        }
        if (of_a_string_or_number) return success;
        why = spelling + " is not built for " + word::spelling_of(declared_as) + " yet -- " + what_containers_have();
        return not_built_yet;
    }

    // A WINDOW'S OWN METHODS, asked of window_calls.hpp and never copied here --
    // the hand-written container set that used to sit above went stale the same
    // afternoon it was written, and one list is the fix for that.
    if (declared_as == word::code_of(1, 6, 18)) {
        if (window_method_arity(method) < 0) {
            // ASKED OF window_calls.hpp AND NOT WRITTEN HERE, which is the same
            // rule the line above already follows for the arity: this file kept
            // its own copy of a method list once and it was stale by the
            // afternoon. A widget added to window_calls.cpp's table appears in
            // this sentence without anybody coming back here.
            why = spelling + " -- " + window_methods_are();
            return types_do_not_meet;
        }
        // HOW `.press` AND `.pressed` ARE SPELLED IS NOT CHECKED HERE, and the
        // reason is a defect this file had for a day (found by a fresh reader,
        // 2026-09-21). This function sees only the FIRST method of a chain on a
        // DECLARED name, so a rule written here holds for exactly one spelling:
        // `satellite.window.button("x").pressed("nosuch")` has no declared
        // receiver and `b.title("t").pressed("nosuch")` is not the first method,
        // and BOTH escaped -- drew a window, and failed at the moment somebody
        // pressed the button. That is precisely the refusal WIN-11 claims to
        // have moved earlier. It now lives in names_in_statement, which walks
        // the WHOLE statement, so every spelling passes through it.
        //
        // HOW MANY IT WAS GIVEN *IS* CHECKED HERE, and it is the RECEIVER's
        // business: `w.title("a", "b")` is a window being asked something a
        // window does not do, and the receiver is what says so. Added
        // 2026-09-21, because `.pressed` had this and its neighbours did not --
        // one method refused before the run and the rest at it, for no reason a
        // person could see.
        //
        // ONLY WITH BRACKETS. `w.title`, `b.pressed` and `w.ok` written bare are
        // a READ, not a call with no arguments, so a missing `(` is not a
        // missing argument -- and `.close` written bare is already told to put
        // its brackets on, in the sentence that says a window DOES it.
        const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
        std::size_t close = k + 2, given = 0;
        if (bracketed && !brackets_at(row, k + 2, close, given)) {
            why = spelling + "( is never closed on its line";
            return satl_line_not_understood;
        }
        // `.press` AND `.pressed` SAY IT BETTER THEMSELVES, wherever they are
        // written, so they are not counted twice and given the duller sentence.
        // `.append` HAS TWO RIGHT COUNTS AND THE CHECKER LETS BOTH THROUGH
        // (GTK-7). It cannot do better: which one is right depends on what the
        // receiver turned out to BE, and a satellite.variable.window name may
        // hold a window or a row. window_append() names the wrong one at the
        // moment it knows, with the piece it actually got.
        if (bracketed && method != token::press_token && !window_method_takes_a_capsule_name(method) &&
            given != static_cast<std::size_t>(window_method_arity(method)) &&
            static_cast<int>(given) != window_method_also_takes(method)) {
            why = spelling + " takes " + window_method_takes(method) + ", and was given " + std::to_string(given);
            return satl_line_not_understood;
        }
        // A SECTION'S NAME, ON A NAME THAT HOLDS A WINDOW (the review, 2026-10-10): judged here and in a
        // widget's chain after one. `.satellite.window.section` since SEC-0, which no object's capsule can be.
        if (method == token::window_section_token)
            return section_written_wrong(row, k + 2, spelling, why);
        return success;
    }

    // A STRING'S OWN METHODS (M16, string_calls.hpp): how many each takes, and a literal
    // that can never be right in its place, judged here so nothing above it prints first.
    if (declared_as == word::code_of(1, 6, 1) && string_method_arity(method) >= 0) {
        std::size_t close = k + 2, given = 0;
        const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
        if (bracketed && !brackets_at(row, k + 2, close, given)) {
            why = spelling + "( is never closed on its line";
            return satl_line_not_understood;
        }
        return string_method_check(row, k + 2, bracketed, given, method, spelling, why);
    }
    // A STRING'S CASE, .upper() and .lower() and their second spellings (string_case.hpp):
    // on a string, taking nothing; anything else has no case to change.
    if (method == token::upper_token || method == token::lower_token) {
        if (declared_as != word::code_of(1, 6, 1)) {
            why = spelling + " is a string's -- " + word::spelling_of(declared_as) + " has no letters to change";
            return types_do_not_meet;
        }
        std::size_t close = k + 2, given = 0;
        if (code_at(row, k + 2) == token::left_parenthesis_token && (!brackets_at(row, k + 2, close, given) || given != 0)) {
            why = spelling + "() takes nothing in its brackets";
            return satl_line_not_understood;
        }
        return success;
    }
    // A STRING'S COLOUR (console_style.hpp): one colour, in brackets, and a literal that
    // cannot be one is refused now.
    if (declared_as == word::code_of(1, 6, 1) &&
        (method == token::foreground_token || method == token::background_token)) {
        std::size_t close = k + 2, given = 0;
        const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
        if (!bracketed || !brackets_at(row, k + 2, close, given) || given != 1) {
            why = spelling + " takes one colour, in brackets: " + spelling + "(xFF8800)";
            return satl_line_not_understood;
        }
        why = colour_literal_refused(row, k + 3, spelling);
        return why.empty() ? success : types_do_not_meet;
    }
    // A BINARY'S, A PERCENTAGE'S, A BOOL'S AND A TIME'S OWN WORDS (scalar_calls.hpp, 2026-10-11): each runs its
    // number, and is let through here as the conversions and .add always were -- a count is judged where it
    // runs, in the words it was always said in, and a word numbered with nothing built behind it (a binary's
    // .width and .clear) is told there that it is not built for a binary yet. A float's, a hex's, a colour's and
    // a fraction's are judged above, by their own checks.
    if (const satelliteObject::Kind scalar = kind_of_type_word(declared_as);
        an_other_scalar(scalar) && number_a_method_runs(scalar, method) != 0)
        return success;
    if (declared_as != word::code_of(1, 6, 2)) {
        if (of_a_string_or_number) return success;
        why = spelling + " is not built for " + word::spelling_of(declared_as) + " yet -- " + so_far_whose(method);
        return not_built_yet;
    }
    if (arity < 0) {
        why = spelling + " -- a file has no " + method_spelling(method) +
              " (SATELLITE_FILE_OPERATIONS Part 3 lists what a file does)";
        return types_do_not_meet;
    }
    std::size_t close = k + 2, given = 0;
    const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
    if (bracketed && !brackets_at(row, k + 2, close, given)) {
        why = spelling + "( is never closed on its line";
        return satl_line_not_understood;
    }
    if (given != static_cast<std::size_t>(arity) || (arity > 0 && !bracketed)) {
        why = spelling + " takes " + std::to_string(arity) + (arity == 1 ? " argument" : " arguments") +
              (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
        return satl_line_not_understood;
    }
    // A LITERAL THAT CAN NEVER BE TEXT WHERE A FILE'S METHOD READS TEXT (the checker gaps of 2026-10-19):
    // f.append(1s) is refused here, in the walker's sentence, and not after the lines above it have printed.
    const int text_slot = method == token::append_token || method == token::index_of_token ||
                                  method == token::search_token || method == token::contains_token ||
                                  method == token::remove_token
                              ? 0
                          : method == token::insert_token || method == token::replace_token ? 1
                                                                                            : -1;
    if (bracketed && text_slot >= 0)
        return text_literal_refused(row, k + 2, arity, text_slot, spelling, why);
    return success;
}

// A BARE WORD THAT MAY BE HIS CAPSULE, where the checker cannot see what the receiver holds (the author's C of
// 2026-10-10: his own spacesuit's methods stay as he writes them): asked of every spacesuit the program has, as
// member() would reach it (CapsuleTable::declares) -- under any spelling of the word, so a capsule str() is called by
// `.string()`; through a spacesuit it extends; and only where this line reaches it, so a protected lock() of his is
// no capsule from outside. `written` is left as written, for the sentence, read back only when no spacesuit has one.
// `hidden` is left naming a spacesuit that HAS one this line cannot reach -- one not in its satellite.public part --
// or empty, so the sentence never says "no spacesuit has" of a capsule his spacesuit does have (the verify of 0040).
bool a_capsule_could_be_called(const Where &where, const std::vector<std::bitset<16>> &row, std::size_t at,
                               std::string &written, std::string &hidden)
{
    const Code method = code_at(row, at);
    const std::string name = token::method_name_of(method);
    for (std::size_t suit = 0; suit < where.capsules.scopes.size(); ++suit)
        if (where.capsules.scopes[suit].is_a_suit() && where.capsules.declares(suit, name, where.scope))
            return true;
    written = method_as_written(row, at, file_of(where), method_spelling(method));
    hidden.clear();
    for (std::size_t suit = 0; suit < where.capsules.scopes.size() && hidden.empty(); ++suit)
        if (where.capsules.scopes[suit].is_a_suit() && where.capsules.declares(suit, name, kNoScope))
            hidden = where.capsules.scopes[suit].name;
    return false;
}

// THE SENTENCE FOR A BARE WORD OF THE LANGUAGE AFTER A RECEIVER ONLY THE RUN CAN TYPE (the author's N of 2026-10-11):
// what to write, and why it is not his -- no spacesuit has a capsule of that name, so it can only be the language's;
// or one has, where this line cannot call it (`hidden`, a_capsule_could_be_called).
std::string bare_word_written(const std::string &receiver, const std::string &word, const std::string &hidden)
{
    return receiver + "." + word + " is written " + receiver + ".satellite." + word + " now -- the language's own "
           "words are under satellite, and " +
           (hidden.empty() ? "no spacesuit of this program has a capsule named " + word
                           : "no capsule named " + word + " is one this line can call -- " + hidden +
                                 "'s is not in its satellite.public part");
}

// A CHAIN'S LATER LINKS, judged as its first is while what each link answers is known
// (method_periods.hpp's kind_a_method_answers): the author's B of 2026-10-10, every call of a chain takes
// its own period -- `s.satellite.upper().satellite.find("X")`. `k` is the first link's period and `kind`
// what the receiver before it is; `spelled` the receiver as written, which grows a link at a time into the
// walker's own name for each turn (link_spelled), so the two say one sentence.
//
// AND ONCE NOTHING HERE KNOWS WHAT A LINK ANSWERED -- an item a list put there, a thread's join(), a capsule's
// answer -- every bare word after it is refused before anything runs, unless some spacesuit has a capsule of that
// name (the author's N of 2026-10-11, "n yes"; his A of that afternoon: "x.pointer() has to return an error unless
// the user has defined a function named "pointer" -- this error has to show up for everything"). Its `.satellite.`
// words and his names are the walker's to judge, and the chain is followed past them. Only with `where`: a caller
// without one is judged as it always was.
signed long long int chain_periods(const std::vector<std::bitset<16>> &row, std::size_t k, satelliteObject::Kind kind,
                                   const std::string &receiver, const std::string &file, std::string &why,
                                   const Where *where = nullptr)
{
    // THE SENTENCE IS SPELLED ONLY WHEN ONE IS SAID (the review): a chain that is right costs its walk and no text.
    const std::size_t first = k;
    const auto spelled_to = [&row, &receiver, first](std::size_t stop) {
        std::string spelled = receiver;
        for (std::size_t at = first; at < stop;) {
            spelled += link_spelled(row, at);
            if (code_at(row, ++at) == token::name_token)
                text_at(row, at);          // a name and its letters
            else
                ++at;
            if (code_at(row, at) == token::left_parenthesis_token) {
                std::size_t close = at, count = 0;
                if (!brackets_at(row, at, close, count)) break;
                at = close + 1;
            }
        }
        return spelled;
    };
    for (;;) {
        // WHAT THE LINK WAS GIVEN, counted before what it answers is asked: a colour's .transparency reads a
        // number and .transparency(50) answers the colour (method_periods.hpp). A NAME is a link only once nothing
        // here knows the chain: his capsule's, or a widget's.
        const token::Code method = code_at(row, k + 1);
        // A WORD A KNOWN KIND DOES NOT HAVE, WRITTEN BARE, is the run's to refuse (his E of 2026-10-11, open) -- and
        // the fault the run names first, so nothing after it is judged as if it had answered something.
        if (kind != satelliteObject::how_many_kinds && method != token::name_token &&
            code_at(row, k) == token::method_token && number_a_method_runs(kind, method) == 0)
            return success;
        std::size_t next = k + 1, given = 0;
        if (method != token::name_token)
            ++next;
        else if (kind != satelliteObject::how_many_kinds)
            return success;
        else
            text_at(row, next);
        if (code_at(row, next) == token::left_parenthesis_token) {
            std::size_t close = next;
            if (!brackets_at(row, next, close, given))
                return success;
            next = close + 1;
        }
        if (kind != satelliteObject::how_many_kinds)
            kind = kind_a_method_answers(kind, method, given);
        if (!a_method_period(code_at(row, next)))
            return success;
        if (kind == satelliteObject::how_many_kinds) {
            if (where == nullptr)
                return success;
            if (code_at(row, next + 1) == token::name_token) {
                // A WIDGET'S NAME is judged where the loop meets it, as a window's (names_in_statement).
                std::size_t name_at = next + 1;
                if (where->widget_names != nullptr && where->widget_names->count(text_at(row, name_at)) != 0)
                    return success;
                k = next;
                continue;
            }
            if (!token::is_method_code(code_at(row, next + 1)))
                return success;
            std::string written, hidden;
            if (code_at(row, next) == token::method_token &&
                !a_capsule_could_be_called(*where, row, next + 1, written, hidden)) {
                why = bare_word_written(spelled_to(next), written, hidden);
                return satl_line_not_understood;
            }
            k = next;
            continue;
        }
        if (!token::is_method_code(code_at(row, next + 1)))
            return success;
        // A WINDOW'S WORD OF PARTS WRITTEN AS ITS PARTS is told the whole word (window_calls.hpp).
        if (kind == satelliteObject::window && a_word_in_parts(row, next)) {
            why = word_in_parts_written_wrong(row, next, spelled_to(next));
            return satl_line_not_understood;
        }
        const bool under = code_at(row, next) == token::satellite_method_token;
        if ((number_a_method_runs(kind, code_at(row, next + 1)) != 0) != under) {
            why = period_written_wrong(spelled_to(next), method_as_written(row, next + 1, file,
                                                                           method_spelling(code_at(row, next + 1))),
                                       kind, under);
            return satl_line_not_understood;
        }
        k = next;
    }
}

// AN OBJECT AN ITEM IS, THROUGH EVERY [ ] -- `squares[key]` on a map<number, land_square>, `grid[1][2]` on a
// list<list<cell>>: the spacesuit its declared shape reaches (a list<T>'s item is a T, a map<K, V>'s value a V),
// with `close` on the last `]` and `spelled` the receiver as a sentence says it (`squares[...]`); kNoSuit when the
// shape reaches no spacesuit. `k` is the first `[`. (The fresh reader of 2026-10-11: such an item's members were
// judged only as their line ran, where a list<suit>'s were judged before anything ran.)
std::size_t suit_an_item_is(const std::vector<std::bitset<16>> &row, std::size_t k, const TypeShape &shape,
                            std::size_t &close, std::string &spelled)
{
    const TypeShape *here = &shape;
    while (code_at(row, k) == token::left_square_bracket_token) {
        std::size_t count = 0;
        if (!brackets_at(row, k, close, count))
            return kNoSuit;
        if (here->word == word::code_of(1, 4, 2) && here->parameters.size() == 1)
            here = &here->parameters[0];
        else if (is_an_index_word(here->word) && here->parameters.size() == 2)
            here = &here->parameters[1];
        else
            return kNoSuit;
        spelled += "[...]";
        k = close + 1;
    }
    return here != &shape && here->is_a_suit() ? here->suit : kNoSuit;
}

signed long long int words_after_an_answer(const std::vector<std::bitset<16>> &row, std::size_t k,
                                           satelliteObject::Kind kind, const std::string &receiver, const Where &where,
                                           std::string &why);

// AN ITEM'S METHOD, judged by what the name was declared to hold (the review of 2026-10-10): `names[1].size` on a
// list<string> is a string's word, so it takes `.satellite.` -- said before anything runs, as a name's own is. Only
// while the declared shape says what each [ ] reaches: an item of a list<T> is a T, a value of a map<K, V> a V, a
// character of a string and a line of a file are text. `k` is the first `[`.
//
// ANYTHING ELSE -- a list with no <type>, a multiple, the arguments, a [ ] past what the shape says -- is what the run
// put there, and a bare word of the language after it is refused before anything runs, unless some spacesuit has a
// capsule of that name (the author's N of 2026-10-11; words_after_an_answer). Only with `where`: without one it waits
// for the run, as it always did.
signed long long int item_periods(const std::vector<std::bitset<16>> &row, std::size_t k, const TypeShape &shape,
                                  const std::string &name, const std::string &file, std::string &why,
                                  const Where *where = nullptr)
{
    satelliteObject::Kind kind = satelliteObject::how_many_kinds;
    const TypeShape *here = &shape;
    std::string spelled = name;
    while (code_at(row, k) == token::left_square_bracket_token) {
        std::size_t close = k, count = 0;
        if (!brackets_at(row, k, close, count))
            return success;
        if (here == nullptr) {
            kind = satelliteObject::how_many_kinds;   // past what the shape says
        } else if (here->word == word::code_of(1, 4, 2) && here->parameters.size() == 1) {
            here = &here->parameters[0];
            kind = kind_of_type_word(here->word);
        } else if (is_an_index_word(here->word) && here->parameters.size() == 2) {
            here = &here->parameters[1];
            kind = kind_of_type_word(here->word);
        } else if (here->word == word::code_of(1, 6, 1) || here->word == word::code_of(1, 6, 2)) {
            here = nullptr;                // a character, or a line of a file: text, with nothing under it
            kind = satelliteObject::string;
        } else if (here->word == word::code_of(1, 6, 22)) {
            here = nullptr;                // an info's item: a map of string keys, its values what the run read
            kind = satelliteObject::index;
        } else {
            here = nullptr;
            kind = satelliteObject::how_many_kinds;
        }
        spelled += "[...]";
        k = close + 1;
    }
    if (kind == satelliteObject::how_many_kinds)
        return where == nullptr ? success : words_after_an_answer(row, k, kind, spelled, *where, why);
    if (!a_method_period(code_at(row, k)) || !token::is_method_code(code_at(row, k + 1)))
        return success;
    if (kind == satelliteObject::window && a_word_in_parts(row, k)) {
        why = word_in_parts_written_wrong(row, k, spelled);
        return satl_line_not_understood;
    }
    const bool under = code_at(row, k) == token::satellite_method_token;
    if ((number_a_method_runs(kind, code_at(row, k + 1)) != 0) != under) {
        why = period_written_wrong(spelled, method_as_written(row, k + 1, file, method_spelling(code_at(row, k + 1))),
                                   kind, under);
        return satl_line_not_understood;
    }
    return chain_periods(row, k, kind, spelled, file, why, where);
}

// A MULTIPLE'S WORD, JUDGED BY THE TYPES IT LISTS (the author's F of 2026-10-11: "yes" -- x.satellite.upper() on a
// multiple holding "abc" was refused as "not built for satellite.container.multiple", and x.satellite.max(9) counted as
// a list's, though the walker runs the word of whatever the multiple holds). Every type between its < and >, or every
// kind there is when it lists none. `.satellite.` before a word some listed type numbers is let through -- what it is
// given and what it answers are the held value's, judged when it runs; a bare `.` before one is refused before anything
// runs, naming its new spelling, unless a spacesuit it lists declares a capsule of that name, which a bare `.` calls
// (his C). `decided` is false for a word no listed type numbers, which goes on to method_judged as it always went.
signed long long int multiples_word(const std::vector<std::bitset<16>> &row, std::size_t k, const std::string &name,
                                    const Where &where, std::string &why, const std::string &file, bool &decided)
{
    decided = false;
    const Code method = code_at(row, k + 1);
    const std::unordered_map<std::string, TypeShape>::const_iterator shape = where.shapes.find(name);
    std::vector<satelliteObject::Kind> kinds;
    std::vector<std::size_t> suits;
    if (shape == where.shapes.end() || shape->second.parameters.empty()) {
        for (std::size_t each = 0; each < satelliteObject::how_many_kinds; ++each)
            kinds.push_back(static_cast<satelliteObject::Kind>(each));
    } else {
        for (const TypeShape &arm : shape->second.parameters) {
            if (arm.is_a_suit())
                suits.push_back(arm.suit);
            else
                kinds.push_back(kind_of_type_word(arm.word));
        }
    }
    satelliteObject::Kind numbers_it = satelliteObject::how_many_kinds;
    for (const satelliteObject::Kind each : kinds)
        if (each != satelliteObject::user_defined && each != satelliteObject::pointer &&
            each != satelliteObject::how_many_kinds && number_a_method_runs(each, method) != 0) {
            numbers_it = each;
            break;
        }
    if (numbers_it == satelliteObject::how_many_kinds)
        return success;
    decided = true;
    if (code_at(row, k) == token::satellite_method_token)
        return success;
    for (const std::size_t suit : suits)
        if (where.capsules.declares(suit, method_spelling(method), where.scope))
            return success;
    why = period_written_wrong(name, method_as_written(row, k + 1, file, method_spelling(method)), numbers_it, false);
    return satl_line_not_understood;
}

// AS WRITTEN: `n.power(2)` is told about n.power, not the registry's n.power_of (ERRORS2 #10).
// ONLY WHEN IT IS REFUSED (the review, 2026-09-26): reading the name back from the text costs
// a look at the whole file, and asked for every method a program has it made the checker's
// time grow with the square of the file -- 8,000 lines of `s = s.lower()` took 17 s, not
// 0.1. A method that passes is judged once, by the registry's name, which it never shows;
// one that is refused is judged again under the name it was written with, for the sentence.
//
// `where`, when given, is the line's place, which a MULTIPLE is judged by (multiples_word, below).
signed long long int method_on_a_name(const std::vector<std::bitset<16>> &row, std::size_t k,
                                      const std::string &name, Code declared_as, std::string &why,
                                      const std::string &file, const Where *where = nullptr)
{
    if (!a_method_period(code_at(row, k)) || !token::is_method_code(code_at(row, k + 1)))
        return success;
    const std::string registry_name = method_spelling(code_at(row, k + 1));
    // A MULTIPLE'S WORD, JUDGED BY THE TYPES IT LISTS (the author's F of 2026-10-11: "yes"), before the rest.
    if (declared_as == word::code_of(1, 4, 6) && where != nullptr) {
        bool decided = false;
        const signed long long int judged = multiples_word(row, k, name, *where, why, file, decided);
        // AND THE REST OF ITS CHAIN (the author's N of 2026-10-11): what the word answered on whatever the multiple
        // holds, or what his capsule handed back, only the run knows.
        if (decided)
            return judged != success
                       ? judged
                       : chain_periods(row, k, satelliteObject::how_many_kinds, name, file, why, where);
    }
    // object.satellite.pointer() AND .satellite.reference() ON ANYTHING BUT AN OBJECT are told they are an
    // object's, in the new spelling whichever period was written -- before the period is judged, which
    // would tell a number to write n.pointer() and then refuse that (method_judged says it; a multiple,
    // which may hold an object, is let through).
    if (code_at(row, k + 1) == token::pointer_token || code_at(row, k + 1) == token::reference_token) {
        const signed long long int judged =
            method_judged(row, k, name, declared_as, why, name + ".satellite." + registry_name);
        if (judged != success)
            return judged;
    }
    // .get AND .set ON THE ARGUMENTS VARIABLE ARE REFUSED WHICHEVER PERIOD THEY ARE WRITTEN WITH, as they were
    // while the two were matched by name (main_arguments.hpp) -- asked first, so `args.get(...)` is not sent to
    // write args.satellite.get(...) and be refused again.
    if (declared_as == word::code_of(1, 6, 21)) {
        const std::string refused = get_or_set_on_the_arguments(code_at(row, k + 1), name);
        if (!refused.empty()) {
            why = refused;
            return satl_line_not_understood;
        }
    }
    // WHICH PERIOD IT TAKES (method_periods.hpp), by what the name was declared as: `.satellite.` for a
    // string's and a list's own words, `.` for everything else -- the walker's rule, said before anything
    // runs. A multiple may hold a string, a list or neither, so its call is judged when it runs.
    const bool under = code_at(row, k) == token::satellite_method_token;
    const satelliteObject::Kind kind = kind_of_type_word(declared_as);
    // A WINDOW'S WORD OF PARTS WRITTEN AS ITS PARTS -- `main_window.widgets.display.file(...)`, the family's old
    // name displays, a scroll alone -- is told the whole word, before the period's rule names only its first part.
    if (kind == satelliteObject::window && a_word_in_parts(row, k)) {
        why = word_in_parts_written_wrong(row, k, name);
        return satl_line_not_understood;
    }
    if (kind != satelliteObject::how_many_kinds &&
        (number_a_method_runs(kind, code_at(row, k + 1)) != 0) != under) {
        why = period_written_wrong(name, method_as_written(row, k + 1, file, registry_name), kind, under);
        return satl_line_not_understood;
    }
    const std::string period = under ? ".satellite." : ".";
    const signed long long int judged = method_judged(row, k, name, declared_as, why, name + period + registry_name);
    if (judged == success)
        return kind == satelliteObject::how_many_kinds && where == nullptr
                   ? success
                   : chain_periods(row, k, kind, name, file, why, where);
    const std::string written = method_as_written(row, k + 1, file, registry_name);
    if (written == registry_name)
        return judged;
    why.clear();
    return method_judged(row, k, name, declared_as, why, name + period + written);
}

// A WINDOW VALUE'S WORD, judged where the checker knows the receiver is one though no name was declared one -- after
// a widget's own name (main_window.files_display), after a window word's answer (satellite.window.button("x")): a
// word of parts whole, the period a window's words take, and every link of the chain after it (NM-8, 2026-10-11).
// `k` is the period; `receiver` the call as written up to it.
signed long long int window_link_periods(const std::vector<std::bitset<16>> &row, std::size_t k,
                                         const std::string &receiver, const std::string &file, std::string &why,
                                         const Where *where = nullptr)
{
    if (a_word_in_parts(row, k)) {
        why = word_in_parts_written_wrong(row, k, receiver);
        return satl_line_not_understood;
    }
    const bool under = code_at(row, k) == token::satellite_method_token;
    if ((number_a_method_runs(satelliteObject::window, code_at(row, k + 1)) != 0) != under) {
        why = period_written_wrong(receiver, method_as_written(row, k + 1, file, method_spelling(code_at(row, k + 1))),
                                   satelliteObject::window, under);
        return satl_line_not_understood;
    }
    return chain_periods(row, k, satelliteObject::window, receiver, file, why, where);
}

// THE WORDS AFTER AN ANSWER THE CHECKER MEETS IN A LINE -- a bracket's value, a capsule's answer, a word's, a {}
// literal, a row of the arguments, an item (the author's N of 2026-10-11, "n yes": a bare word after any of them was
// refused only as its line ran, after the lines above it had printed). `k` is the period after it; `kind` what it is
// where that is known here, and how_many_kinds where only the run knows; `receiver` the answer as the walker names
// it -- "that value", "that answer", satellite.file.new.
//
// A KNOWN KIND takes its own period, as a declared name of it does: a word it does not have at all stays for the run
// (his E of 2026-10-11, left open). AN UNKNOWN ONE: a bare `.` before a word of the language is refused, unless some
// spacesuit of the program has a capsule of that name, which a bare `.` calls (his C) -- his A of that afternoon:
// "x.pointer() has to return an error unless the user has defined a function named "pointer" -- this error has to
// show up for everything". Its `.satellite.` words are the walker's to judge. Every later link the same way
// (chain_periods). A widget's name after it is judged where the loop meets it, as a window's.
signed long long int words_after_an_answer(const std::vector<std::bitset<16>> &row, std::size_t k,
                                           satelliteObject::Kind kind, const std::string &receiver, const Where &where,
                                           std::string &why)
{
    if (!a_method_period(code_at(row, k)))
        return success;
    const Code word = code_at(row, k + 1);
    if (word == token::name_token) {
        std::size_t name_at = k + 1;
        if (kind != satelliteObject::how_many_kinds ||
            (where.widget_names != nullptr && where.widget_names->count(text_at(row, name_at)) != 0))
            return success;
        return chain_periods(row, k, kind, receiver, file_of(where), why, &where);
    }
    if (!token::is_method_code(word))
        return success;
    if (kind == satelliteObject::window)
        return window_link_periods(row, k, receiver, file_of(where), why, &where);
    const bool under = code_at(row, k) == token::satellite_method_token;
    if (kind == satelliteObject::how_many_kinds) {
        std::string written, hidden;
        if (!under && !a_capsule_could_be_called(where, row, k + 1, written, hidden)) {
            why = bare_word_written(receiver, written, hidden);
            return satl_line_not_understood;
        }
    } else if ((number_a_method_runs(kind, word) != 0) != under) {
        why = period_written_wrong(receiver, method_as_written(row, k + 1, file_of(where), method_spelling(word)), kind,
                                   under);
        return satl_line_not_understood;
    }
    return chain_periods(row, k, kind, receiver, file_of(where), why, &where);
}

// THE WORDS AFTER A ROW OF THE ARGUMENTS (the author's N of 2026-10-11): the row keeps its own name -- his ruling of
// that afternoon, satellite.arguments.username -- and a word of the language after it goes under satellite:
// `.username.upper()`, `.username.size` (split from the row as the walker splits it, a_word_after_the_rows),
// `.cpu.features[1].upper()`, `["username"].upper()`. A row satl holds has a kind the checker can read
// (kind_of_a_row_satl_holds), so its words are judged as a declared name's are; a program's own row, or a key worked
// out, is what only the run can type. `from` is the `.` or the `[` after the arguments' name `name`, `rows_end` where
// the dotted run of its row names ends.
signed long long int words_after_the_rows(const std::vector<std::bitset<16>> &row, std::size_t from,
                                          std::size_t rows_end, const std::string &name, const Where &where,
                                          std::string &why)
{
    std::string key;
    satelliteObject::Kind kind = satelliteObject::how_many_kinds;
    const std::size_t word = a_word_after_the_rows(row, from, where.arguments, where.functions, key, kind);
    if (word != from)
        return words_after_an_answer(row, word, kind, name + "." + key, where, why);
    // NOTHING AFTER THE ROW BUT ITS VALUE'S END -- the common read -- asks no row its kind.
    if (!a_method_period(code_at(row, rows_end)) && code_at(row, rows_end) != token::left_square_bracket_token)
        return success;
    past_the_argument_names(row, from, key);
    kind = key.empty() ? satelliteObject::how_many_kinds
                       : kind_of_a_row_satl_holds(key, where.arguments, where.functions);
    std::string spelled = key.empty() ? name : name + "." + key;
    std::size_t k = rows_end;
    while (code_at(row, k) == token::left_square_bracket_token) {
        std::size_t close = k, count = 0;
        if (!brackets_at(row, k, close, count))
            return success;
        // A LIST ROW OF satl'S HOLDS TEXT (the_value_of_a_row), and a character of text is text.
        kind = kind == satelliteObject::list || kind == satelliteObject::string ? satelliteObject::string
                                                                                 : satelliteObject::how_many_kinds;
        spelled += "[...]";
        k = close + 1;
    }
    return words_after_an_answer(row, k, kind, spelled, where, why);
}

// WHAT A WORD'S CALL ANSWERS, where the checker can say it before anything runs (the author's N of 2026-10-11) -- as
// the walker's word answers it, each row proved on BUILD 0039 by a probe of the word's answer: a file word's new file;
// whether a file is there, and whether a file was cleared or the folder changed, a bool; an empty list, an empty map
// and an info's read; the console's input as text; a folder's free space a float; a drawn number and the seed handed
// back; a time; infinity; and a display's -- with its .satellite.center() -- the number 0. how_many_kinds for every
// other word, whose answer only the run knows: a window word's is judged by the window's; a word not built, or one
// whose count is refused, never reaches here; and satellite.directory.list(), which reads its folder and then cannot
// hand a program the names (M14), and satellite.time.sleep, which answers nothing, are left to the run.
satelliteObject::Kind kind_a_word_answers(Code code)
{
    using O = satelliteObject;
    if (code == word::code_of(1, 8, 1) || code == word::code_of(1, 8, 2) || code == word::code_of(1, 8, 4) ||
        code == word::code_of(1, 8, 6))
        return O::file;
    if (code == word::code_of(1, 8, 5) || code == word::code_of(1, 8, 3) || code == word::code_of(1, 18, 1) ||
        code == word::code_of(1, 18, 8) || code == word::code_of(1, 8, 7) || code == word::code_of(1, 8, 8) ||
        code == word::code_of(1, 8, 9) || code == word::code_of(1, 8, 10) || code == word::code_of(1, 8, 11))
        return O::boolean;
    // THE WHOLE PATH LEXES FIRST (bytecode_registry.cpp): satellite.container.list() is the word 1 4 2 with its
    // brackets, never its path() row 1 4 2 0 -- so a word is known here by its family, which holds both.
    if (code == word::fixed_code<1, 4, 1> || code == word::fixed_code<1, 4, 1, 0>)
        return O::index;
    if (is_container_word(code) || code == word::code_of(1, 30, 1) || code == word::code_of(1, 30, 2))
        return O::list;
    if (code == word::code_of(1, 5, 2) || code == word::code_of(1, 5, 3))
        return O::string;
    if (code == word::code_of(1, 18, 7) || code == word::code_of(1, 35, 2))   // ... and satellite.convert.float(x)
        return O::floating;
    if (code == word::code_of(1, 35, 1) || code == word::code_of(1, 35, 3))   // satellite.convert.number(x), .whole(x)
        return O::number;
    if (is_infinity_word(code))
        return O::infinity;              // satellite.infinity(x), not built, is refused before this is asked
    for (int draw = 4; draw <= 12; ++draw)       // fast, normal and ultra: digits, a range, a range and a step
        if (code == word::code_of(1, 7, draw))
            return O::number;
    if (code == word::code_of(1, 7, 17))
        return O::number;
    if (code == word::code_of(1, 9, 5) || code == word::code_of(1, 9, 6) || code == word::code_of(1, 9, 7))
        return O::time;
    // THE CONSOLE'S DOINGS answer the number 0, as a display does: its clear() and home(). (Its foreground() and
    // background() lex as their bare words, which their colour-taking calls share, so they are left to the run.)
    if (is_display_word(code) || code == word::code_of(1, 5, 8) || code == word::code_of(1, 5, 9))
        return O::number;
    return O::how_many_kinds;
}

// WHAT A METHOD-CALL STATEMENT MAY BE, WHOLE (the review: `f.size = 3`, `f.`,
// `f[` and `f.append("x") f.append("y")` passed the check and failed after
// earlier lines had printed). From `k`, any run of `.method`, `.method(...)` and
// `[...]`, and then the line's end: nothing a call answers can be given a value,
// and one statement is one line.
signed long long int a_call_to_its_end(const std::vector<std::bitset<16>> &row, std::size_t k,
                                       const std::string &name, std::string &why)
{
    // WHAT THE RUN ENDED ON, which is the whole of how `a[1] = x` is told from
    // `f.size = 3`. Both are a name, a run of somethings, and an `=`. The first
    // is an assignment into a list and the second is giving a value to a call's
    // answer, which is meaningless -- and the difference is only that one ended
    // on `]` and the other on a method.
    bool ended_on_an_index = false;
    for (;;) {
        std::size_t close = k, count = 0;
        // A METHOD, OR A NAME AFTER A DOT: an object's capsule (2026-09-22), judged by
        // names_in_statement against the spacesuit it belongs to.
        if (a_method_period(code_at(row, k)) &&
            (token::is_method_code(code_at(row, k + 1)) || code_at(row, k + 1) == token::name_token)) {
            if (code_at(row, k + 1) == token::name_token) {
                ++k;
                skip_payload(row, k);
            } else {
                k += 2;
            }
            if (code_at(row, k) == token::left_parenthesis_token) {
                if (!brackets_at(row, k, close, count)) break;
                k = close + 1;
            }
            ended_on_an_index = false;
            continue;
        }
        if (code_at(row, k) == token::left_square_bracket_token) {
            if (!brackets_at(row, k, close, count)) break;
            k = close + 1;
            ended_on_an_index = true;
            continue;
        }
        break;
    }
    const Code code = code_at(row, k);
    if (code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token)
        return success;
    // `a[i] = v`, and `a[i][j] = v` (the author, 2026-09-18). The walker's
    // run_indexed_assignment does the work; here it is only a shape to allow.
    if (code == token::assign_token && ended_on_an_index)
        return success;
    why = name + " is followed by something that is not a method call -- a call's answer cannot be given a "
                 "value, a bracket must close on its line, and one statement is one line";
    return satl_line_not_understood;
}

// HOW MANY A CAPSULE CALL WAS GIVEN, before anything runs (2026-09-21). Until
// capsules took arguments there was nothing to count. `open` is the call's `(`, and
// `written` is the name as the call wrote it -- `greet`, or `other.greet`.
signed long long int given_what_it_takes(const std::vector<std::bitset<16>> &row, std::size_t open,
                                         const CapsuleSite &site, const std::string &written, std::string &why)
{
    std::size_t close = open, given = 0;
    if (!brackets_at(row, open, close, given)) {
        why = written + "( is never closed on its line";
        return satl_line_not_understood;
    }
    const std::size_t takes = site.parameters.size();
    if (given != takes) {
        why = written + " takes " + std::to_string(takes) +
              (takes == 1 ? " argument, and was given " : " arguments, and was given ") + std::to_string(given);
        return satl_line_not_understood;
    }
    return success;
}

// AND WHAT A PRESS CAN HAND IT (2026-09-21). A press has nobody to write its
// arguments -- the program said `.pressed(name)` and walked away -- so the
// capsule's own declaration is what says what it wants, and there are only three
// things a press has to give: nothing, the piece, and the window it is in.
// `method` is the one the program wrote -- .pressed, .changed, .every -- because
// being told about presses after writing `.changed` is being told about somebody
// else's line ("a press" was the truth until GTK-9).
signed long long int a_press_can_run(const CapsuleSite &site, const std::string &name, Code method, std::string &why)
{
    const std::vector<CapsuleParameter> &wants = site.parameters;
    if (wants.size() > 2) {
        why = name + " takes " + std::to_string(wants.size()) + " arguments, and .satellite." +
              std::string(token::method_name_of(method)) + " has only two to give: write " + name + "(), " + name +
              "(satellite.variable.window the_piece), or " + name +
              "(satellite.variable.window the_piece, satellite.variable.window its_window)";
        return satl_line_not_understood;
    }
    for (const CapsuleParameter &takes : wants) {
        if (takes.declared() == word::code_of(1, 6, 18))
            continue;
        why = name + "'s " + takes.name + " is declared " + word::spelling_of(takes.declared()) + ", and .satellite." +
              std::string(token::method_name_of(method)) +
              " hands it the piece it happened to and the window it happened in -- "
              "both are satellite.variable.window";
        return types_do_not_meet;
    }
    return success;
}

// A VARIABLE MAY NOT TAKE THE NAME OF A FILE OR A SPACE IT CAN SEE (2026-09-22). A
// variable called `other`, in a file that includes other.satl, would make
// `other.greet()` a method on the variable and hide the file -- so the name is
// refused where it is declared, which is the one line that can change. 003 refused
// the same collision the other way round (its S1602): a name is declared once.
signed long long int a_name_it_may_take(const CapsuleTable &capsules, std::size_t scope, const std::string &name,
                                        std::string &why)
{
    // `arguments` WAS KEPT FROM EVERY NAME BUT satellite.main's PARAMETER from 2026-10-04, while the bare name was
    // the run's own arguments in every capsule: a body that declared it after reading the run's own under it read
    // one line two ways on a loop's second pass (the fresh reader, 2026-10-06). The run's own are
    // satellite.arguments since 2026-10-11, which no name can be, and the bare name reads nothing of the run's --
    // so it is a name like any other (main_arguments.hpp).
    const std::string taken = capsules.already_names(scope, name);
    if (taken.empty())
        return success;
    // A FILE OR A SPACE IS REACHED THROUGH A DOT, so the dot is what goes ambiguous; a
    // capsule or a spacesuit is just a second thing with one name (M5, DESIGN §7).
    const bool reached_by_a_dot = taken.rfind("the satellite.namespace", 0) == 0 || taken.rfind("the file", 0) == 0;
    why = name + " is already " + taken + ", so a variable or an object cannot be named " + name + " -- " +
          (reached_by_a_dot ? name + ".something() could then mean either"
                            : std::string("a name is declared once, so rename one of them"));
    return name_declared_twice;
}

// THE SENTENCE FOR A NAME A SECTION'S OR A DISPLAY'S LINE DECLARED, declared again by a declaration (SEC-0, rule 4;
// the plan's wording, which is the author's to change): `top` is the section's name, and a line writes it bare.
std::string declared_by_its_line(const std::string &name, const std::string &made_by)
{
    return name + " is declared by its " + made_by + " line already -- write " + name + ", bare, where the " + made_by +
           " is wanted";
}

// A NAME DECLARED A SECOND TIME, said for what it already is: a field of the object the
// capsule runs on, or another variable of the capsule. `maker`, when a section's or a
// display's line is what declares it now (SEC-0, rule 4): "section", "file display".
std::string declared_twice(const Where &where, const std::string &name, const char *maker = nullptr)
{
    if (deleted_in_another_block(where, name))
        return name + " was deleted by satellite.delete(" + name + ") on line " +
               std::to_string(where.deleted_on_line.at(name)) + ", inside a block that has ended -- a deleted name "
               "is free again only inside the block that deleted it, so here it would be declared twice in the "
               "same capsule";
    if (where.site != nullptr && where.site->suit != kNoScope) {
        const CapsuleScope &suit = where.capsules.scopes[where.site->suit];
        if (suit.layout->slot_of(name) != kNoSlot)
            return name + " is a field of " + suit.layout->shown + ", and a name is declared once -- a " +
                   (maker != nullptr ? std::string(maker) + " made in" : "variable or a parameter of") +
                   " its capsule cannot be named " + name + " as well";
    }
    // A SECTION'S OR A DISPLAY'S NAME IS A NAME (SEC-0): two sections of one name in one capsule are refused, even in
    // two windows, and so is a name the capsule has already -- a variable, a parameter -- given to one.
    const std::unordered_map<std::string, std::string>::const_iterator made = where.made_by.find(name);
    if (made != where.made_by.end())
        return maker == nullptr ? declared_by_its_line(name, made->second)
                                : name + " is declared by its " + made->second + " line already, and a name is "
                                  "declared once in a capsule -- give this " + maker + " a name of its own";
    if (maker != nullptr)
        return name + " is declared in this capsule already, and a " + maker + "'s name is a name, declared by its "
               "line -- give the " + maker + " a name of its own";
    return name + " is declared twice in the same capsule";
}

// A SPACESUIT'S CAPSULE CALLED BY ITS BARE NAME runs on the object the calling capsule
// runs on -- so the line must stand in a capsule of THAT spacesuit (2026-09-22). Not in
// a field's value, which is worked out before there is an object, and not in a spacesuit
// declared inside it, whose capsules run on an object of their own spacesuit.
signed long long int a_capsule_with_an_object(const CapsuleSite &site, const Where &where,
                                              const std::string &written, std::string &why)
{
    if (site.suit == kNoScope)
        return success;
    if (where.field) {
        why = written + " is a capsule of the spacesuit " + where.capsules.scopes[site.suit].within + ", and a field's "
              "value is worked out before there is an object to run it on";
        return satl_line_not_understood;
    }
    // A SUPERTYPE'S CAPSULE runs on an object of a spacesuit that extends it just as well.
    const bool extends_it = where.site != nullptr && where.site->suit != kNoScope &&
                            where.capsules.scopes[where.site->suit].layout->is_a(site.suit);
    if (where.site == nullptr || (where.site->suit != site.suit && !extends_it)) {
        why = written + " is a capsule of the spacesuit " + where.capsules.scopes[site.suit].within + ", and it runs "
              "on an object of it -- call it on one: an_object." + site.name + "(...)";
        return satl_line_not_understood;
    }
    return success;
}

// IS `name` A CAPSULE OF ANY SPACESUIT -- asked of a name after a `.` whose object is an
// answer, known only running: `shards[t].call_live()`, `make().call_x()`.
bool some_suit_has(const CapsuleTable &capsules, const std::string &name)
{
    for (const CapsuleScope &scope : capsules.scopes)
        if (scope.is_a_suit() && scope.capsules.count(name) != 0)
            return true;
    return false;
}

// AND ONE THIS LINE CAN REACH (OPEN_ITEMS_2026-10-20, the checker gaps of 2026-10-19: `a.make().secret()`): when every
// spacesuit that has a capsule of that name keeps it in its satellite.protected part, the call is refused before
// anything runs, S230, as `b.secret()` is through a named object -- `a.make().secret()` printed the lines above it
// first. Whose object the answer is is known only running, so the sentence names every spacesuit that keeps it
// protected and holds for any answer: one whose spacesuit has no such capsule cannot be asked for it either. One
// public one anywhere leaves the call to the run, which knows whose object the answer is (the satellite.spacesuit
// help: "a protected capsule reached from outside").
signed long long int a_capsule_this_line_reaches(const Where &where, const std::string &name, std::string &why)
{
    const CapsuleTable &capsules = where.capsules;
    const token::Code as_a_method = token::method_code_of(name);
    std::vector<std::string> keeping;   // the spacesuits whose own capsules hold it, protected
    for (std::size_t suit = 0; suit < capsules.scopes.size(); ++suit) {
        const CapsuleScope &of = capsules.scopes[suit];
        if (!of.is_a_suit()) continue;
        if (capsules.declares(suit, name, where.scope)) return success;
        if (!capsules.declares(suit, name, kNoScope)) continue;
        bool own = of.capsules.count(name) != 0;
        for (auto spelled = of.capsules.begin(); !own && as_a_method != 0 && spelled != of.capsules.end(); ++spelled)
            own = token::method_code_of(spelled->first) == as_a_method;
        if (own) keeping.push_back(of.layout != nullptr ? of.layout->shown : of.within);
    }
    if (keeping.empty()) return success;
    std::string parts = keeping.front();
    for (std::size_t k = 1; k < keeping.size(); ++k)
        parts += (k + 1 == keeping.size() ? " and of " : ", of ") + keeping[k];
    why = name + " is inside the satellite.protected part of " + parts + ", so only " +
          (keeping.size() == 1 ? keeping.front() + "'s" : std::string("their")) + " own capsules can reach it, and no "
          "spacesuit has a " + name + " this line can reach -- whatever object comes before the ., its " + name +
          "() cannot be called here";
    return member_is_protected;
}

// A CAPSULE CALLED WHERE ITS ANSWER IS USED (2026-09-22): given what it takes, reachable
// with an object when it is a spacesuit's, and able to hand a value back at all.
// `open` is the call's `(`.
signed long long int a_call_for_its_answer(const std::vector<std::bitset<16>> &row, std::size_t open,
                                           const CapsuleSite &site, const std::string &written, const Where &where,
                                           std::string &why)
{
    signed long long int held = given_what_it_takes(row, open, site, written, why);
    if (held != success)
        return held;
    held = a_capsule_with_an_object(site, where, written, why);
    if (held != success)
        return held;
    if (!hands_back_a_value(where.registry, site)) {
        why = written + "() is used where its answer would be, and it never hands one back -- no satellite.return(...) "
                        "in it has a value";
        return capsule_gave_no_answer;
    }
    return success;
}

// `obj.name(...)` ON A NAME THAT HOLDS AN OBJECT OF `suit` (2026-09-22). `k` is on the
// `.` and is left on the call's `(`, so the arguments are judged as the loop goes on.
// `receiver_at` is where `obj` stands: a call that IS the whole statement lets its
// answer go, and anywhere else the answer is used.
//
// AND ITS PERIOD (pointer_calls.hpp; NM-8, 2026-10-11): `.satellite.` before the five words every
// object has, and a bare `.` before a capsule of his -- which a name his spacesuit declares always is,
// one of the five too (the author's C of 2026-10-10). A multiple's five are refused bare before anything runs as
// an object's are -- a file's are under satellite too (2026-10-11; the author's F that evening: a multiple judged by
// the types it lists), so nothing it may hold takes them bare.
signed long long int member_of_an_object(const std::vector<std::bitset<16>> &row, std::size_t &k,
                                         std::size_t receiver_at, const std::string &receiver, std::size_t suit,
                                         const Where &where, std::string &why)
{
    const bool under = code_at(row, k) == token::satellite_method_token;
    std::size_t m = k + 1;
    std::string member;
    if (code_at(row, m) == token::name_token) {
        member = text_at(row, m);
    } else if (token::is_method_code(code_at(row, m))) {
        member = token::method_name_of(code_at(row, m));
        ++m;
    } else {
        why = receiver + " is followed by a . and nothing an object of " +
              where.capsules.scopes[suit].layout->shown + " has";
        return satl_line_not_understood;
    }
    const std::string written = receiver + (under ? ".satellite." : ".") + member;
    const bool one_of_the_five = code_at(row, k + 1) != token::name_token && object_number_of(code_at(row, k + 1)) != 0;
    // ASKED ONLY WHERE IT DECIDES THE PERIOD: an ordinary `obj.call_x()` is looked up once, by member() below.
    // A BARE ONE OF THE FIVE IS HIS ONLY WHERE THIS LINE REACHES HIS CAPSULE OF THAT NAME (CapsuleTable::declares):
    // from outside, a protected lock() of his leaves `d.lock()` the language's word written the old way.
    const bool his = (under || one_of_the_five) &&
                     where.capsules.declares(suit, member, under ? kNoScope : where.scope);
    // THE FIVE WORDS EVERY OBJECT HAS (pointer_calls.hpp), whatever its spacesuit declares, each taking
    // nothing: after `.satellite.` always, and after a bare `.` when his spacesuit has no capsule of that
    // name -- refused then, naming the new spelling, as the walker refuses it.
    if (one_of_the_five && (under || !his)) {
        if (!under) {
            why = period_written_wrong(receiver, member, satelliteObject::user_defined, false);
            return satl_line_not_understood;
        }
        if (code_at(row, m) != token::left_parenthesis_token || code_at(row, m + 1) != token::right_parenthesis_token) {
            why = written + "() takes nothing, in its brackets";
            return satl_line_not_understood;
        }
        const std::size_t period = k;
        k = m;
        // AND THE REST OF ITS CHAIN, before anything runs (the author's B of 2026-10-10: every call of a
        // chain takes its own period). pointer() and reference() answer an object of this same
        // spacesuit, whose next link is judged as this one was; ok() answers a bool, judged as one is.
        const std::size_t next = m + 2;
        if (!a_method_period(code_at(row, next)) ||
            (code_at(row, next + 1) != token::name_token && !token::is_method_code(code_at(row, next + 1))))
            return success;
        if (code_at(row, period + 1) == token::pointer_token || code_at(row, period + 1) == token::reference_token) {
            std::size_t link = next;
            return member_of_an_object(row, link, receiver_at, written + "()", suit, where, why);
        }
        return token::is_method_code(code_at(row, next + 1))
                   ? chain_periods(row, period, satelliteObject::user_defined, receiver, file_of(where), why, &where)
                   : success;
    }
    // A CAPSULE OF HIS WRITTEN UNDER SATELLITE: his own spacesuit's methods stay as he writes them (his C).
    if (under && his) {
        why = period_written_wrong(receiver, member, satelliteObject::user_defined, true);
        return satl_line_not_understood;
    }
    signed long long int code = success;
    const CapsuleSite *site = where.capsules.member(suit, member, where.scope, code, why);
    if (site == nullptr)
        return code;
    if (code_at(row, m) != token::left_parenthesis_token) {
        why = written + " is a capsule of " + where.capsules.scopes[suit].layout->shown +
              ", and a capsule is called with its brackets: " + written + "()";
        return satl_line_not_understood;
    }
    const signed long long int held = given_what_it_takes(row, m, *site, written, why);
    if (held != success)
        return held;
    std::size_t close = m, given = 0;
    brackets_at(row, m, close, given);
    const Code after = code_at(row, close + 1);
    const bool stands_alone = where.statement == receiver_at &&
                              (after == token::line_end_token || after == token::comment_token ||
                               after == token::end_of_file_token);
    if (!stands_alone && !hands_back_a_value(where.registry, *site)) {
        why = written + "() is used where its answer would be, and it never hands one back -- no satellite.return(...) "
                        "in it has a value";
        return capsule_gave_no_answer;
    }
    // AND THE WORDS AFTER WHAT HIS CAPSULE HANDS BACK, which only the run can type (the author's N of 2026-10-11) --
    // named as the walker names it, `h.call_inner()`.
    const signed long long int answered = words_after_an_answer(row, close + 1, satelliteObject::how_many_kinds,
                                                                written + (given == 0 ? "()" : "(...)"), where, why);
    if (answered != success)
        return answered;
    k = m;
    return success;
}

// WHAT A VALUE WORD ANSWERS -- a word with no brackets that answers a value, which a word may follow as one follows
// a literal (the author's G of 2026-10-11: "yes" -- satellite.bool.true.satellite.string,
// satellite.console.width.satellite.string, satellite.library.arguments.username.satellite.upper()): a bool word and
// a setting read by its bare name answer a bool, the console's width and height a number, and a fact about the
// machine what its library says -- asked here as it is when its line runs, since a fact only reads. how_many_kinds
// for any other word, and for a fact that could not be read, which the walker then says.
satelliteObject::Kind kind_a_value_word_answers(Code code, const FunctionTable &functions)
{
    if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2))
        return satelliteObject::boolean;
    if (is_console_fact(code))
        return satelliteObject::number;
    const NumberRow *library = functions[code];
    if (library == nullptr)
        return satelliteObject::how_many_kinds;
    if (library->scenarios.flag_setting != nullptr)
        return satelliteObject::boolean;
    if (library->scenarios.fact != nullptr) {
        const FactReply said = library->scenarios.fact();
        if (said.code == success)
            return said.is_text ? satelliteObject::string : satelliteObject::number;
    }
    return satelliteObject::how_many_kinds;
}

// Every name a statement USES as a value -- so a name with no declaration is
// caught before anything runs. A name followed by `(` is a capsule and is
// checked against the capsule table instead.
signed long long int names_in_statement(const std::vector<std::bitset<16>> &row,
                                        std::size_t from,
                                        std::size_t stop,
                                        const DeclaredNames &declared,
                                        const Where &where,
                                        std::string &why)
{
    const CapsuleTable &capsules = where.capsules;
    const std::size_t scope = where.scope;
    const FunctionTable &functions = where.functions;
    // THE TWO CODES THIS LOOP LAST VISITED, and NOT row[at - 1] and row[at - 2]
    // (WIN-11). `.pressed(when_pressed)` is recognised by what stands before the
    // name, and a raw index backwards can land INSIDE A PAYLOAD -- where a
    // string's characters are their own Unicode numbers and one of them may
    // equal a token's code exactly. That is not a hypothetical in this tree:
    // capsules_in() carries the same warning, for a string ending in U+1006 that
    // made the next body a second satellite.main. This loop already SKIPS every
    // payload, so the codes it visited are the only ones that are really tokens.
    Code one_back = 0, two_back = 0;
    std::vector<std::size_t> judged;     // members judged through a list of objects
    std::vector<std::size_t> option_names;   // `foreground` in foreground=..., judged by its word
    std::vector<std::pair<std::size_t, std::size_t>> options_under;   // satellite.end=: where it starts, its value
    for (std::size_t at = from; at < stop && at < row.size(); ) {
        const Code code = code_at(row, at);
        const Code before_this = one_back, and_before_that = two_back;
        two_back = one_back;
        one_back = code;

        // THE ARGUMENTS' OLD PLACE IS REFUSED BEFORE ANYTHING RUNS (the author's K of 2026-10-11):
        // satellite.library.main.arguments.<row>, where they lived until 2026-10-03, is satellite.arguments.<row>.
        if (word::is_word_code(code)) {
            static const std::string old_place = "satellite.library.main.arguments";
            const char *spelled = word::spelling_of(code);
            if (spelled != nullptr && std::string_view(spelled).substr(0, old_place.size()) == old_place) {
                std::string written = spelled;
                if (const std::size_t open = written.find('('); open != std::string::npos)
                    written.erase(open);
                why = written + " is written satellite.arguments" + written.substr(old_place.size()) +
                      " now -- satellite.library.main.arguments is where the arguments lived until 2026-10-03";
                return satl_line_not_understood;
            }
        }

        // A TOUCHING SIGN IS REFUSED WHEREVER IT IS WRITTEN (ERRORS4 022), in the walker's own sentences
        // (expression.cpp, refuse_if_reserved): `n = n+1` in an if never taken ran to exit 0, and `display(4+3)`
        // was refused only after the lines above it had printed. This loop visits every code of every statement
        // and skips every payload, so a code here is a real sign. A for's step is for_step_moves_by's, and a %
        // pressed against a number's digits is a percentage before it is ever a sign (the lexer).
        if ((code == token::tight_plus_token || code == token::tight_times_token || code == token::tight_power_token ||
             code == token::tight_modulus_token) &&
            (where.for_step == kNowhere || at < where.for_step)) {
            if (code == token::tight_times_token && code_at(row, at + 1) == token::tight_times_token) {
                why = "power is written with a space on both sides -- 2 ** 3 or 2 ^ 3, never 2**3";
                return satl_line_not_understood;
            }
            why = std::string("every math operation is written with a space on both sides of the sign, and this ") +
                  (code == token::tight_plus_token    ? "+"
                   : code == token::tight_times_token ? "*"
                   : code == token::tight_power_token ? "^"
                                                      : "%") +
                  " has none";
            return satl_line_not_understood;
        }

        // A NAMED OPTION WRITTEN UNDER SATELLITE -- satellite.end= -- is stepped over whole, to its value: the word,
        // its period and the name are the option's, judged with its word below (the author's B of 2026-10-11).
        bool an_option_under = false;
        for (const auto &[starts, value] : options_under)
            if (starts == at) {
                at = value;
                an_option_under = true;
                break;
            }
        if (an_option_under) {
            one_back = token::assign_token;
            continue;
        }

        // HOW `.pressed(...)` IS SPELLED, WHEREVER IT STANDS AND WHATEVER IT IS
        // WRITTEN ON (WIN-11). A method is judged by its RECEIVER in
        // method_on_a_name, and that is the wrong place for this: a receiver
        // that is a word's answer has no declared name, and a chain's second
        // method is never reached. Here there is no receiver to be gated on --
        // this loop walks every statement and skips every payload, so a
        // `pressed_token` it visits is a real one, wherever it was written.
        // THE AUTHOR'S WIDGET WORDS (2026-10-09), judged wherever they are written: the old family's parts -- a
        // `.displays`, a `.file` or an `.input`, or a `.widgets` or a `.display` with the rest of the word after it --
        // and SINCE SEC-0 the old one-code words .satellite.widgets.display.file and .satellite.widgets.display.input
        // (SATELLITE_WINDOW_SECTIONS.md rule 8), which only a window ever had: each is refused, on whatever it was
        // written on, naming the word it is now (window_calls.hpp says the sentence). `.section` is not asked here:
        // an object's capsule may be called that. A name declared a window, or anything else whose kind is known,
        // was told already with its own name; here there is no receiver to name.
        if (a_method_period(before_this) && a_method_period(code_at(row, at - 1)) &&
            (code == token::displays_token || code == token::file_piece_token || code == token::input_piece_token ||
             code == token::widgets_display_file_token || code == token::widgets_display_input_token ||
             (code == token::widgets_token && a_method_period(code_at(row, at + 1)) &&
              (code_at(row, at + 2) == token::display_token || code_at(row, at + 2) == token::displays_token)) ||
             (code == token::display_token && a_word_in_parts(row, at - 1)))) {
            why = word_in_parts_written_wrong(row, at - 1, "");
            return satl_line_not_understood;
        }
        // `.listen()` IS `.listen` WITH NO CAPSULE (the author, 2026-10-09): nothing in its brackets is
        // the other right shape, and the rule below is for the shape with a name.
        if (code == token::listen_token && code_at(row, at + 1) == token::left_parenthesis_token &&
            code_at(row, at + 2) == token::right_parenthesis_token) {
            at += 3;
            continue;
        }
        if (window_method_takes_a_capsule_name(code) &&
            code_at(row, at + 1) == token::left_parenthesis_token) {
            const std::string spelled = std::string(".satellite.") + token::method_name_of(code);
            std::size_t argument = at + 2;
            // TEXT IS NOT A NAME. It would lex, check, run, open the window, and
            // fail at the moment of the press -- window already up.
            if (code_at(row, argument) != token::name_token) {
                why = spelled + " takes the NAME of a capsule, written as it is written: " + spelled +
                      "(when_pressed) -- not text and not a value worked out, because satl proves "
                      "the capsule is there before your program runs";
                return types_do_not_meet;
            }
            // AND ONE NAME, NOT A NAME AND THEN ANYTHING. Looking only at the
            // code after the `(` let `.pressed(when_pressed, 5)` through to be
            // refused at run time, which is a refusal this checker owes earlier.
            // ONE NAME MAY BE DOTTED (2026-09-22): `.pressed(other.go)` names the
            // capsule go in the file other.satl, and is still one name.
            std::vector<std::string> dotted;
            dotted_names_at(row, argument, dotted);
            // AND WHAT MAY FOLLOW IT IS THE METHOD'S BUSINESS, asked of
            // window_calls.hpp (GTK-13). `.pressed`, `.changed` and `.closed`
            // take one name and nothing else; `.every` takes the name and then
            // how often, so a comma is what it wants there.
            const bool more_may_follow = window_method_takes_more_after_the_name(code);
            const token::Code after = code_at(row, argument);
            if (!more_may_follow && after != token::right_parenthesis_token) {
                why = spelled + " takes one capsule's name and nothing else";
                return satl_line_not_understood;
            }
            if (more_may_follow && after != token::comma_token) {
                why = spelled + " takes a capsule's NAME first and then the rest: " +
                      (code == token::ask_token    ? spelled + "(when_answered, \"delete it?\")"
                       : code == token::item_token ? spelled + "(when_open, \"Open\")"
                                                   : spelled + "(when_it_ticks, 1000)");
                return satl_line_not_understood;
            }
        }
        // `.press()` IS THE OTHER ONE, AND THEY ARE ONE LETTER APART -- so the
        // refusal for either given the other's argument names the other out
        // loud, rather than leaving a person with "when_pressed has no
        // satellite.variable line declaring it": a true sentence about the wrong
        // half of a typo.
        if (code == token::press_token && code_at(row, at + 1) == token::left_parenthesis_token &&
            code_at(row, at + 2) != token::right_parenthesis_token) {
            why = ".satellite.press() is the PROGRAM pressing it, and a click has nothing to say, so it takes "
                  "nothing. To name what a press RUNS, that is .satellite.pressed(a_capsule)";
            return satl_line_not_understood;
        }

        // A satellite.library VALUE READ IN THE LINE (library_values.hpp): its names judged
        // here and stepped over, so a method after them is judged as the loop goes on.
        if (is_library_word(code) && code_at(row, at + 1) == token::method_token) {
            std::string written;
            Code type = 0;
            const signed long long int read = library_read_is_right(capsules, where.registry, row, at, written, type, why);
            if (read != success) return read;
            const signed long long int method = method_on_a_name(row, at, written, type, why, file_of(where), &where);
            if (method != success) return method;
            continue;
        }

        // THE RUN'S OWN ARGUMENTS, satellite.arguments (the author, 2026-10-11, of what the bare name `arguments`
        // read from 2026-10-04: "arguments.missing or arguments.anything needs to be able to be read anywhere"):
        // its rows are names, as main's are, and which rows there are is the run's to say -- the walker says it.
        // A method on the whole of it is judged as on main's; one that changes it in place is refused, because
        // what it would change is a copy nobody keeps -- and asked of EITHER period, or
        // satellite.arguments.satellite.clear() slipped past (method_periods.hpp's a_method_period).
        if (code == kTheRunsArgumentsWord) {
            std::size_t k = at + 1;
            std::string key;
            std::size_t open = k;
            if (code_at(row, k) == token::left_parenthesis_token) {
                why = std::string(kTheRunsArguments) + " is not a call -- the run's own arguments are read row by row, "
                      "as satellite.arguments.username, and shown whole as satellite.arguments";
                return satl_line_not_understood;
            }
            // A SETTING WRITTEN BY ITS CALL, INSIDE AN EXPRESSION -- `display(satellite.arguments.missing(false))`:
            // the call changes a row and answers nothing, so it is a line of its own (the fresh reader, 2026-10-06).
            if (an_argument_call(row, k, key, open)) {
                why = std::string(kTheRunsArguments) + "." + key + "(...) changes a row of the arguments and answers "
                      "nothing, so it is a line of its own -- the row is read as " + kTheRunsArguments + "." + key;
                return satl_line_not_understood;
            }
            const std::size_t rows_end = past_the_argument_names(row, k);
            if (rows_end != k && !a_map_word_where_a_row_would_be(row, k)) {
                // AND THE WORDS AFTER THE ROW (the author's N of 2026-10-11): the row keeps its name, the word moves.
                const signed long long int after =
                    words_after_the_rows(row, k, rows_end, kTheRunsArguments, where, why);
                if (after != success) return after;
                k = rows_end;
            } else if (a_method_period(code_at(row, k)) && token::is_method_code(code_at(row, k + 1)) &&
                       changes_a_container(code_at(row, k + 1))) {
                why = std::string(kTheRunsArguments) + ".satellite." + token::method_name_of(code_at(row, k + 1)) +
                      "(...) would change a copy of the run's own rows that nothing keeps -- satellite.arguments is "
                      "read, and a setting of it changed with satellite.arguments.<row>(value)";
                return satl_line_not_understood;
            } else if (code_at(row, k) == token::left_square_bracket_token) {
                // A ROW BY ITS KEY, satellite.arguments["username"], and the words after it (N, as above).
                const signed long long int after = words_after_the_rows(row, k, k, kTheRunsArguments, where, why);
                if (after != success) return after;
            } else {
                const signed long long int judged_here =
                    method_on_a_name(row, k, kTheRunsArguments, word::code_of(1, 6, 21), why, file_of(where), &where);
                if (judged_here != success) return judged_here;
            }
            at = k;
            continue;
        }

        // A BRACKET'S VALUE AND A {} LITERAL, AND THE WORDS AFTER THEM (the author's N of 2026-10-11, "n yes"):
        // `(5).floor()` and `{1, 2}.size` were refused only as their line ran. A `(` with no name, word or method
        // straight before it is no call's: it holds "that value", as the walker names it, which only the run can
        // type (words_after_an_answer). A `{` where a value goes is a list, or a map when a `:` follows its first item.
        if (code == token::left_parenthesis_token && before_this != token::name_token &&
            !token::is_method_code(before_this) && !word::is_word_code(before_this)) {
            std::size_t close = at, count = 0;
            if (brackets_at(row, at, close, count) && a_method_period(code_at(row, close + 1))) {
                const signed long long int after = words_after_an_answer(
                    row, close + 1, satelliteObject::how_many_kinds, "that value", where, why);
                if (after != success) return after;
            }
        }
        if (code == token::left_brace_token) {
            std::size_t close = at;
            bool a_map = false;
            if (braces_at(row, at, close, a_map) && a_method_period(code_at(row, close + 1))) {
                const signed long long int after =
                    words_after_an_answer(row, close + 1, a_map ? satelliteObject::index : satelliteObject::list,
                                          a_map ? "that map" : "that list", where, why);
                if (after != success) return after;
            }
        }

        if (code == token::name_token) {
            std::size_t k = at;
            const std::string name = text_at(row, k);

            // A NAMED OPTION'S NAME (console_calls.hpp) is the option and not a variable:
            // judged by name with its word, below, and stepped over here with its `=`, so
            // its value is judged as the loop goes on.
            bool an_option = false;
            for (const std::size_t each : option_names) an_option = an_option || each == at;
            if (an_option) {
                // ITS `=` IS WHAT STANDS BEFORE ITS VALUE, so a bracket opening the value is a bracket's, not a call's
                // (the author's N of 2026-10-11: end=("!").upper() was judged only as its line ran).
                one_back = token::assign_token;
                at = k + 1;
                continue;
            }
            // AND ONE NOBODY TAKES: `twice(x=2)` gives a capsule an option, and a capsule's
            // arguments are given in order. Said as that, not as a variable x nobody declared
            // -- and ONLY straight after a call's `(`, a name's or a method's: after a comma
            // it may be `satellite.variable.number a = 1, b = 2`, and after a bare `(` it may
            // be `while((i = 3) > 0)`, and both are judged as they always were (the review).
            if (before_this == token::left_parenthesis_token &&
                (and_before_that == token::name_token || token::is_method_code(and_before_that)) &&
                code_at(row, k) == token::assign_token) {
                why = name + "= is a named option, and only satellite.console.display and satellite.console.input "
                             "take them -- a capsule is given its arguments in order, without names";
                return satl_line_not_understood;
            }

            // A MEMBER ALREADY JUDGED, through the list its object came out of (below).
            bool already_judged = false;
            for (const std::size_t each : judged) already_judged = already_judged || each == at;
            if (already_judged && before_this == token::method_token) {
                at = k;
                continue;
            }

            // A CAPSULE'S NAME INSIDE A LINE is a call for its answer (2026-09-22) -- a
            // statement that is only a call is judged by check_statement, which hands this
            // function its arguments alone -- or, after `.pressed(`, the capsule a button
            // runs, written as a name and never called here.
            const bool names_what_a_press_runs =
                before_this == token::left_parenthesis_token && window_method_takes_a_capsule_name(and_before_that);
            const bool after_a_dot = before_this == token::method_token;
            std::vector<std::string> names;
            std::size_t past = at;
            dotted_names_at(row, past, names);
            std::string written = names.front();
            for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];

            // WHAT A PRESS RUNS IS ONE CAPSULE, AND THE WHOLE NAME MUST REACH IT --
            // `.pressed(go)`, `.pressed(other.go)`. Judging only the first name let
            // `.pressed(go.reverse)` through to fail at the press (the review).
            if (names_what_a_press_runs) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr) {
                    why = reached.through_a_scope || reached.why.rfind("no capsule named ", 0) != 0
                              ? reached.why
                              : "no capsule named " + written + " -- .satellite." +
                                    std::string(token::method_name_of(and_before_that)) +
                                    "(...) names a capsule to run, and there is no satellite.capsule " + written +
                                    "() in this program";
                    return satl_line_not_understood;
                }
                // A SPACESUIT'S CAPSULE RUNS ON AN OBJECT, and a press has none to give it
                // (the review, 2026-09-22: it passed here and would have run with no object).
                if (reached.site->suit != kNoScope) {
                    why = written + " is a capsule of the spacesuit " + capsules.scopes[reached.site->suit].within +
                          ", and it runs on an object -- .satellite." +
                          std::string(token::method_name_of(and_before_that)) +
                          "(...) names a capsule of a file or a satellite.namespace, which a press runs with none";
                    return satl_line_not_understood;
                }
                const signed long long int fits = a_press_can_run(*reached.site, written, and_before_that, why);
                if (fits != success)
                    return fits;
                at = past;
                continue;
            }

            // A WIDGET BY ITS NAME AFTER AN ITEM OR AN ANSWER -- `ws[1].files`, `m["a"].files`,
            // `ws.satellite.first.files`, `make().files`: a name after a `]`, a `)` or a word's answer that the
            // program gives a widget in quotes somewhere is that widget's, as the walker reads it (a_member_next,
            // a_widget_by_name), and is judged as one after a declared window's name is, below: every name after it a
            // widget's, and the word after them a window's (window_link_periods). A name no widget has is told what
            // it always was. (The sweep of 2026-10-11: ws[1].files.satellite.directory("/tmp/") was refused as an
            // undeclared variable, so no spelling of it could run.)
            if (after_a_dot && where.widget_names != nullptr && where.widget_names->count(name) != 0 &&
                code_at(row, k) != token::left_parenthesis_token &&
                (and_before_that == token::right_square_bracket_token ||
                 and_before_that == token::right_parenthesis_token || token::is_method_code(and_before_that))) {
                std::size_t m = k;
                std::string reached = name;
                while (code_at(row, m) == token::method_token && code_at(row, m + 1) == token::name_token) {
                    std::size_t n = m + 1;
                    const std::string member = text_at(row, n);
                    if (where.widget_names->count(member) == 0) {
                        why = reached + "." + member + " -- no widget is named " + member + " in this program: "
                              ".satellite.window.section(\"" + member + "\", 100%, 50%), "
                              ".satellite.window.display.file(\"" + member + "\") or "
                              ".satellite.window.display.text.input(\"" + member + "\") makes one";
                        return name_not_declared;
                    }
                    reached += "." + member;
                    m = n;
                }
                if (a_method_period(code_at(row, m)) && token::is_method_code(code_at(row, m + 1))) {
                    const signed long long int linked =
                        window_link_periods(row, m, reached, file_of(where), why, &where);
                    if (linked != success)
                        return linked;
                    if (code_at(row, m + 1) == token::window_section_token) {
                        const signed long long int judged =
                            section_written_wrong(row, m + 2, reached + ".satellite.window.section", why);
                        if (judged != success)
                            return judged;
                    }
                }
                at = m;
                continue;
            }

            // A MEMBER OF AN OBJECT -- `log.call_open()`, `plan.call_threads()`, `log.satellite.lock()` --
            // judged by the spacesuit the name holds an object of (2026-09-22), and its period with it:
            // his capsules with a `.`, the five words every object has with `.satellite.` (member_of_an_object).
            // `k` comes back on the call's `(`, so its arguments are judged as the loop goes on.
            if (!after_a_dot && a_method_period(code_at(row, k)) && declared.count(name) != 0) {
                const std::size_t *suit = nullptr;
                const DeclaredObjects::const_iterator object = where.objects.find(name);
                if (object != where.objects.end())
                    suit = &object->second;
                else if (code_at(row, k + 1) == token::name_token || object_number_of(code_at(row, k + 1)) != 0 ||
                         (named_until_numbered(code_at(row, k + 1)) &&
                          code_at(row, k) != token::satellite_method_token)) {
                    // A MULTIPLE WITH A SPACESUIT AMONG ITS TYPES: its capsules, and the words every
                    // object answers (pointer_calls.hpp) -- the review of 2026-10-02 found m.pointer()
                    // refused here as "not an object's" though m held one. AND THE ELEVEN NAMES A NUMBER'S
                    // WORDS MADE CODES OF (number_calls.hpp, 2026-10-11), let through as the names they were:
                    // `m.round()` with a `.` is his spacesuit's round, since a number's is m.satellite.round() --
                    // which goes on to the multiple's own judging (multiples_word), not to his capsules (his F).
                    const DeclaredObjects::const_iterator multiple = where.multiples.find(name);
                    if (multiple != where.multiples.end()) {
                        suit = &multiple->second;
                    }
                }
                if (suit != nullptr) {
                    const signed long long int judged =
                        member_of_an_object(row, k, at, name, *suit, where, why);
                    if (judged != success) return judged;
                    at = k;
                    continue;
                }
                // A WIDGET BY ITS NAME, AFTER A WINDOW (the author, 2026-10-09): `main_window.file_display`
                // and `main_window.file_display.text_input`, each name one the program gives a widget in
                // quotes somewhere (check_program read them all). HIS OWN NAMES KEEP THEIR PERIOD (2026-10-10
                // ~21:15). What follows the names is judged as the loop goes on -- `.listen(when_typed)`
                // names a capsule, wherever it stands.
                if (code_at(row, k + 1) == token::name_token &&
                    declared.find(name)->second == word::code_of(1, 6, 18)) {
                    std::size_t m = k;
                    std::string reached = name;
                    while (code_at(row, m) == token::method_token && code_at(row, m + 1) == token::name_token) {
                        std::size_t n = m + 1;
                        const std::string member = text_at(row, n);
                        if (where.widget_names != nullptr && where.widget_names->count(member) == 0) {
                            why = reached + "." + member + " -- no widget is named " + member + " in this program: "
                                  ".satellite.window.section(\"" + member + "\", 100%, 50%), "
                                  ".satellite.window.display.file(\"" + member + "\") or "
                                  ".satellite.window.display.text.input(\"" + member + "\") makes one";
                            return name_not_declared;
                        }
                        reached += "." + member;
                        m = n;
                    }
                    // AND THE WORD AFTER THEM, ON THE WIDGET THEY REACH -- a window value, so a window's word:
                    // written `.satellite.` (his line, 2026-10-10 ~21:15, in SEC-0's spelling:
                    // main_window.files_display.satellite.window.display.file("list_of_files")), a word of parts
                    // whole, a section made in one with its name in quotes --
                    // main_window.top.satellite.window.section("inner", 50%, 50%) -- and every link of its chain.
                    if (a_method_period(code_at(row, m)) && token::is_method_code(code_at(row, m + 1))) {
                        const signed long long int linked =
                            window_link_periods(row, m, reached, file_of(where), why, &where);
                        if (linked != success)
                            return linked;
                        if (code_at(row, m + 1) == token::window_section_token) {
                            const signed long long int judged =
                                section_written_wrong(row, m + 2, reached + ".satellite.window.section", why);
                            if (judged != success)
                                return judged;
                        }
                    }
                    at = m;
                    continue;
                }
                // `.call_x(` ON A NAME THAT HOLDS NO OBJECT: the name after its dot is not
                // one of its methods -- the lexer made no method code of it -- and only an
                // object has capsules to call.
                if (code_at(row, k + 1) == token::name_token &&
                    declared.find(name)->second != word::code_of(1, 6, 21)) {
                    std::size_t m = k + 1;
                    const std::string member = text_at(row, m);
                    why = name + " is " + word::spelling_of(declared.find(name)->second) + ", and " + member +
                          " is not one of its methods -- only an object of a satellite.spacesuit has capsules to call";
                    return satl_line_not_understood;
                }
            }

            // A CAPSULE CALLED INSIDE A LINE, FOR ITS ANSWER (2026-09-22) -- `other.greet(`,
            // `tools.x(`. Asked only of a name no variable has -- a variable is a method's
            // receiver -- and never of a name after a `.`. When the first name is neither a
            // file nor a space this says nothing, and the name is judged below as it always was.
            if (!after_a_dot && names.size() >= 2 && declared.find(name) == declared.end() &&
                code_at(row, past) == token::left_parenthesis_token) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site != nullptr) {
                    const signed long long int judged = a_call_for_its_answer(row, past, *reached.site, written, where, why);
                    if (judged != success) return judged;
                    // AND THE WORDS AFTER ITS ANSWER, which only the run can type (the author's N of 2026-10-11).
                    std::size_t close = past, given = 0;
                    if (brackets_at(row, past, close, given)) {
                        const signed long long int after = words_after_an_answer(
                            row, close + 1, satelliteObject::how_many_kinds, "that answer", where, why);
                        if (after != success) return after;
                    }
                    at = past;
                    continue;
                }
                if (reached.through_a_scope) {
                    why = reached.why;
                    return reached.code;
                }
            }

            if (code_at(row, k) == token::left_parenthesis_token) {
                // AFTER A `.`, A NAME IS A MEMBER of what came before it. A declared name's
                // members were judged above; this one's object is an ANSWER --
                // `shards[t].call_live()`, `make().call_x()` -- whose spacesuit is known only
                // running, so here it must be a capsule of SOME spacesuit, and the walker
                // judges the rest. Anything else is told what it was always told.
                if (after_a_dot && some_suit_has(capsules, name)) {
                    const signed long long int reached = a_capsule_this_line_reaches(where, name, why);
                    if (reached != success) return reached;
                    at = k;
                    continue;
                }
                const Reached reached = capsules.reach(scope, {name});
                if (reached.site == nullptr) {
                    // A SPACESUIT'S NAME WITH BRACKETS AFTER IT: an object made inside a
                    // line, which neither 003 nor 004 has -- one is made by declaring it.
                    std::string unused;
                    if (!after_a_dot && capsules.suit_named(scope, {name}, unused) != kNoScope) {
                        why = name + " is a spacesuit, and an object of it is made by declaring one on a line of its "
                                     "own -- " + name + " a_name(...) -- and then using a_name";
                        return satl_line_not_understood;
                    }
                    why = after_a_dot ? "no capsule named " + name : reached.why;
                    return satl_line_not_understood;
                }
                if (after_a_dot) {
                    why = "." + name + "(...) is not a method -- " + name + " is a capsule, and a capsule is called "
                          "by its own name, as " + reached.site->shown + "()";
                    return satl_line_not_understood;
                }
                const signed long long int judged = a_call_for_its_answer(row, k, *reached.site, name, where, why);
                if (judged != success) return judged;
                // AND THE WORDS AFTER ITS ANSWER, which only the run can type (the author's N of 2026-10-11).
                std::size_t close = k, given = 0;
                if (brackets_at(row, k, close, given)) {
                    const signed long long int after = words_after_an_answer(
                        row, close + 1, satelliteObject::how_many_kinds, "that answer", where, why);
                    if (after != success) return after;
                }
                at = k;
                continue;
            } else if (!after_a_dot && declared.find(name) != declared.end() &&
                       declared.find(name)->second == word::code_of(1, 6, 21) &&
                       [&] { std::string key; std::size_t open = k; return an_argument_call(row, k, key, open); }()) {
                // A SETTING WRITTEN BY ITS CALL, INSIDE AN EXPRESSION -- `display(arguments.missing(false))`: the call
                // changes a row and answers nothing, so it is a line of its own (the fresh reader, 2026-10-06: it
                // was told "no capsule named missing").
                std::string key;
                std::size_t open = k;
                an_argument_call(row, k, key, open);
                why = name + "." + key + "(...) changes a row of the arguments and answers nothing, so it is a line of "
                                         "its own -- the row is read as " + name + "." + key;
                return satl_line_not_understood;
            } else if (!after_a_dot && name == kTheOldBareArguments && declared.find(name) == declared.end()) {
                // THE BARE NAME THE RUN'S OWN ARGUMENTS HAD UNTIL 2026-10-11, where this body has no name called
                // that: refused before anything runs, naming the new spelling (main_arguments.hpp). The run's own
                // are satellite.arguments, judged above this loop's names.
                why = the_bare_arguments_refused(row, k);
                return name_not_declared;
            } else if (declared.find(name) == declared.end()) {
                // A FIELD'S VALUE NAMING ANOTHER FIELD: there is no object yet (003's S0511).
                const bool a_field = where.field && capsules.scopes[scope].layout != nullptr &&
                                     capsules.scopes[scope].layout->slot_of(name) != kNoSlot;
                // `satellite.machine.cores()`: a row of main's arguments, spelled as a word (ERRORS2 #9).
                const bool after_satellite = before_this == token::method_token &&
                                             (and_before_that == word::kFirst || and_before_that == word::code_of(1, 22));
                const std::string a_row =
                    after_satellite ? arguments_row_written_as_a_word(row, at, and_before_that != word::kFirst) : "";
                why = a_field ? name + " is a field of " + capsules.scopes[scope].layout->shown +
                                    ", and a field's value is worked out before there is an object -- it cannot name "
                                    "another field; give it its value in the satellite.constructor instead"
                      : !a_row.empty() ? a_row
                                       : not_declared_why(where, row, name);
                return name_not_declared;
            } else if (declared.find(name)->second == word::code_of(1, 6, 21) &&
                       past_the_argument_names(row, k) != k && !a_map_word_where_a_row_would_be(row, k)) {
                // THE ARGUMENTS VARIABLE'S ROWS ARE NAMES, NOT VARIABLES:
                // `arguments.memory.total` is one row (main_arguments.hpp), and which
                // rows there are is the machine's to say, so the walker says it. A map's
                // word where a row would be is its method, judged below (2026-10-11).
                // AND THE WORDS AFTER THE ROW (the author's N of 2026-10-11): `args.username.upper()`.
                const std::size_t rows_end = past_the_argument_names(row, k);
                const signed long long int after = words_after_the_rows(row, k, rows_end, name, where, why);
                if (after != success) return after;
                k = rows_end;
            } else if (where.lists.count(name) != 0 && code_at(row, k) == token::left_square_bracket_token) {
                // `units[i].call_x()` -- AN ITEM'S MEMBER, judged by the spacesuit the list was
                // declared to hold (the review, 2026-09-22: it was judged only running). The
                // index's own names are judged as the loop goes on; the member's name, when
                // the loop reaches it, is skipped as judged.
                // EITHER PERIOD: member_of_an_object judges which the member takes.
                std::size_t close = k, count = 0;
                if (brackets_at(row, k, close, count) && a_method_period(code_at(row, close + 1)) &&
                    (code_at(row, close + 2) == token::name_token || token::is_method_code(code_at(row, close + 2)))) {
                    std::size_t dot = close + 1;
                    const signed long long int member =
                        member_of_an_object(row, dot, at, name + "[...]", where.lists.at(name), where, why);
                    if (member != success) return member;
                    judged.push_back(close + 2);
                }
            } else if (where.objects.count(name) == 0) {
                // `s["x"]`, `s[1.5]`, `s[0]` ON A STRING (M16): a literal no character is at,
                // refused as s[n] refuses it, before anything runs (string_calls.hpp).
                if (declared.find(name)->second == word::code_of(1, 6, 1) &&
                    code_at(row, k) == token::left_square_bracket_token) {
                    const signed long long int indexed = string_index_check(row, k, name, why);
                    if (indexed != success) return indexed;
                }
                // AN ITEM'S METHOD, by the shape the name was declared with (item_periods) -- AND AN ITEM THAT IS AN
                // OBJECT, a map's value or one two brackets down, judged as a list<suit>'s item is above: his
                // capsules after `.`, the five words every object has after `.satellite.` (member_of_an_object).
                const std::unordered_map<std::string, TypeShape>::const_iterator shape = where.shapes.find(name);
                if (code_at(row, k) == token::left_square_bracket_token && shape != where.shapes.end() &&
                    shape->second.word == declared.find(name)->second) {
                    std::size_t close = k;
                    std::string spelled = name;
                    const std::size_t suit = suit_an_item_is(row, k, shape->second, close, spelled);
                    if (suit != kNoSuit && a_method_period(code_at(row, close + 1)) &&
                        (code_at(row, close + 2) == token::name_token || token::is_method_code(code_at(row, close + 2)))) {
                        std::size_t dot = close + 1;
                        const signed long long int member =
                            member_of_an_object(row, dot, at, spelled, suit, where, why);
                        if (member != success) return member;
                        judged.push_back(close + 2);
                    } else {
                        const signed long long int item =
                            item_periods(row, k, shape->second, name, file_of(where), why, &where);
                        if (item != success) return item;
                    }
                } else if (code_at(row, k) == token::left_square_bracket_token) {
                    // AN ITEM OF A NAME WHOSE SHAPE IS NOT KEPT, judged by its declared type alone: a character of a
                    // string, a line of a file -- and, for anything else, what only the run can type (the author's N
                    // of 2026-10-11), a bare word after it refused unless some spacesuit has a capsule of that name.
                    TypeShape declared_shape;
                    declared_shape.word = declared.find(name)->second;
                    const signed long long int item =
                        item_periods(row, k, declared_shape, name, file_of(where), why, &where);
                    if (item != success) return item;
                }
                const signed long long int judged_here =
                    method_on_a_name(row, k, name, declared.find(name)->second, why, file_of(where), &where);
                if (judged_here != success) return judged_here;
            }
            at = k;
            continue;
        }

        // satellite.thread.new(capsule(args)) (2026-09-23, thread_calls.hpp): what is inside
        // is a CALL to one of the program's own capsules, KEPT and not run -- so it is judged
        // as a call (it reaches a capsule; it is given what it takes) and never as a call for
        // its answer: a capsule that hands nothing back is what a thread most often runs.
        // Its arguments are judged as the loop goes on, from the call's `(`.
        if (is_thread_word(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            std::size_t past = at + 2;
            if (code_at(row, past) != token::name_token) {
                why = "satellite.thread.new runs a capsule of your own on a thread, so what goes inside it is a "
                      "call: my_capsule() or my_capsule(x)";
                return thread_needs_a_capsule_call;
            }
            std::vector<std::string> names;
            dotted_names_at(row, past, names);
            std::string written = names.front();
            for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];
            // A WORD UNDER SATELLITE IS THE LANGUAGE'S, NEVER A CAPSULE OF YOURS (NM-8, 2026-10-11):
            // `satellite.thread.new(x.satellite.pointer())` is told that, as the walker tells it -- the names
            // stop at `.satellite.`, and were told they name a capsule without calling it.
            if (code_at(row, past) == token::satellite_method_token) {
                const std::string word = token::method_name_of(code_at(row, past + 1));
                why = not_a_capsule_for_a_thread(written + ".satellite." + word, word);
                return thread_needs_a_capsule_call;
            }
            if (code_at(row, past) != token::left_parenthesis_token) {
                why = "satellite.thread.new(" + written + ") names a capsule and does not call it -- write " +
                      written + "(), with what it takes inside the brackets";
                return thread_needs_a_capsule_call;
            }
            // `obj.call_x(...)` -- A CAPSULE OF AN OBJECT, run on a thread on that object (T2:
            // shared, and made safe by the object's own .lock()).
            const CapsuleSite *site = nullptr;
            const DeclaredObjects::const_iterator object =
                names.size() == 2 ? where.objects.find(names.front()) : where.objects.end();
            if (object != where.objects.end()) {
                // ONE OF THE FIVE IS NO CAPSULE TO RUN -- unless his spacesuit declares a capsule of that
                // name, which a bare `.` calls everywhere (pointer_calls.hpp; the author's C of 2026-10-10).
                if (every_object_word(names.back()) && !capsules.declares(object->second, names.back(), scope)) {
                    why = not_a_capsule_for_a_thread(written, names.back());
                    return thread_needs_a_capsule_call;
                }
                signed long long int refused = success;
                site = capsules.member(object->second, names.back(), scope, refused, why);
                if (site == nullptr)
                    return refused;
            } else {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr) {
                    why = reached.why;
                    return satl_line_not_understood;
                }
                site = reached.site;
                // A SPACESUIT'S CAPSULE BY ITS BARE NAME runs on this body's object, so the
                // line must stand in a capsule of that spacesuit, as a call must.
                const signed long long int with_an_object = a_capsule_with_an_object(*site, where, written, why);
                if (with_an_object != success)
                    return with_an_object;
            }
            const signed long long int given = given_what_it_takes(row, past, *site, written, why);
            if (given != success)
                return given;
            std::size_t close = past, count = 0;
            brackets_at(row, past, close, count);
            if (code_at(row, close + 1) != token::right_parenthesis_token) {
                why = "satellite.thread.new takes one capsule call and nothing after it";
                return satl_line_not_understood;
            }
            // AND THE WORDS AFTER IT, ON THE THREAD IT ANSWERS (the author's N of 2026-10-11): judged before the thread
            // is made, where `satellite.thread.new(w()).start()` was refused only after it had been.
            const std::string spelled_word(word::spelling_of(code));
            const signed long long int after =
                words_after_an_answer(row, close + 2, satelliteObject::thread,
                                      spelled_word.substr(0, spelled_word.find('(')) + "(...)", where, why);
            if (after != success) return after;
            at = past;
            continue;
        }

        // satellite.access(name) (2026-09-26, access_calls.hpp): what is inside is a NAME,
        // read by its declaration and never worked out as a value -- one name, nothing else,
        // which the loop then judges as declared from the `(`, as it judges every name.
        if (is_access_word(code)) {
            const std::string refused = access_refused(row, at);
            if (!refused.empty()) {
                why = refused;
                return satl_line_not_understood;
            }
            ++at;
            continue;
        }

        // satellite.delete INSIDE ANOTHER LINE: it answers nothing, so it is a line of its own
        // (check_statement takes the line that begins with it before this loop sees it).
        if (is_delete_word(code)) {
            why = kDeleteIsALineOfItsOwn;
            return satl_line_not_understood;
        }

        // satellite.console.width AND .height ARE READ, NOT CALLED (console_calls.hpp),
        // so `width()` is told that rather than that it has no library.
        if (is_console_fact(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            // SAID AS WHAT TO WRITE. It was "x is read with no brackets: x", which read as an
            // accusation about a line that plainly has brackets (the error sweep, 2026-09-25).
            why = std::string(word::spelling_of(code)) + " is read without brackets -- write " +
                  word::spelling_of(code) + ", not " + word::spelling_of(code) + "()";
            return satl_line_not_understood;
        }

        // satellite.error() IS A BLOCK, NOT A VALUE (ERROR_BLOCK_PLAN.md): said as where it goes, never as a word
        // still owed a library.
        if (code == word::code_of(1, 37)) {
            why = std::string("satellite.error() answers nothing -- ") + kErrorWithNoBlock;
            return satl_line_not_understood;
        }

        // A WORD USED AS A CALL MUST HAVE A LIBRARY. A word with none is
        // not_built_yet (14) with its own name, which is what 003 did and what a
        // person can act on (function_table.hpp).
        // satellite.file's words, satellite.infinity(), satellite.window's and
        // satellite.container.list() are the object model's and have none
        // (file_calls.hpp, infinity_calls.hpp, window_calls.hpp, container_calls.hpp)
        // -- each of them answers a HANDLE, which is the one thing a library cannot
        // make.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token &&
            functions[code] == nullptr && !is_file_word(code) && !is_infinity_word(code) &&
            !is_window_word(code) && !is_container_word(code) && !is_console_word(code) && !is_info_word(code) &&
            !is_random_word(code) && !is_test_word(code) && !is_time_word(code) && !is_convert_word(code) &&
            !is_drive_word(code)) {
            why = std::string(word::spelling_of(code)) + " has no library built for it yet";
            return not_built_yet;
        }

        // HOW MANY ARGUMENTS A WORD WAS GIVEN, judged here and not after the lines
        // above it have run (the review, 2026-09-18). A file word takes its own
        // count; a library takes one -- call_word hands it one value, and
        // `satellite.variable.string.replace("a", "b")` lexed to a two-parameter row
        // with a library once the lexer began counting commas.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            std::size_t close = at + 1, given = 0;
            if (brackets_at(row, at + 1, close, given)) {
                const std::string spelled_word(word::spelling_of(code));
                // .center() ON A DISPLAY IS UNDER SATELLITE (NM-8, 2026-10-11): it is the word
                // satellite.console.display.center, 1 5 1 1, written satellite.console.display(x).satellite.center()
                // -- a bare `.` is refused, naming it, before anything runs, as the walker refuses it.
                if (is_display_word(code) && code_at(row, close + 1) == token::method_token &&
                    display_number_of(code_at(row, close + 2)) != 0) {
                    why = center_written_bare(spelled_word.substr(0, spelled_word.find('(')),
                                              method_as_written(row, close + 2, file_of(where),
                                                                method_spelling(code_at(row, close + 2))));
                    return satl_line_not_understood;
                }
                // NAMED OPTIONS, `foreground=xFF8800` (console_calls.hpp): judged by name
                // and by literal here, never counted as arguments, and their names
                // stepped over as options rather than judged as variables nobody declared.
                // Not in satellite.statement's own brackets, where `(d = 0; ...)` starts a loop.
                if (spelled_word.rfind("satellite.statement.", 0) != 0) {
                    std::vector<WrittenOption> options;
                    if (!options_written_in(row, at + 1, options, why)) {
                        why = spelled_word.substr(0, spelled_word.find('(')) + " -- " + why;
                        return satl_line_not_understood;
                    }
                    for (std::size_t n = 0; n < options.size(); ++n) {
                        const signed long long int refused = option_refused(code, row, options, n, why);
                        if (refused != success) return refused;
                        if (options[n].under)
                            options_under.emplace_back(options[n].name_at, options[n].value_at);
                        else
                            option_names.push_back(options[n].name_at);
                    }
                    given -= options.size();
                }
                // A COLOUR WORD GIVEN A LITERAL THAT CANNOT BE A COLOUR, said now.
                if (a_colour_word_given_one(code) && given == 1) {
                    const std::string refused =
                        colour_literal_refused(row, at + 2, spelled_word.substr(0, spelled_word.find('(')));
                    if (!refused.empty()) {
                        why = refused;
                        return types_do_not_meet;
                    }
                }
                // satellite.infinity(x) is infinity ** x, and INF-5 builds it.
                const std::string not_yet = infinity_word_not_built(code, given);
                if (!not_yet.empty()) {
                    why = not_yet;
                    return not_built_yet;
                }
                // satellite.container.list() takes nothing: a list that holds
                // something is written with braces.
                const std::string no_arguments = container_word_refused(code, given);
                if (!no_arguments.empty()) {
                    why = no_arguments;
                    return satl_line_not_understood;
                }
                // satellite.random's words take their own counts (random_calls.hpp), and
                // the bare grades -- fast(), normal(), ultra() -- draw nothing: S430 here,
                // before anything runs, as 003 ruled it; seeded is not built; and
                // satellite.random itself is not a call.
                if (is_random_word(code)) {
                    const signed long long int refused = random_word_refused(code, given, why);
                    if (refused != success)
                        return refused;
                }
                // satellite.drive.list() takes nothing (drive_calls.hpp).
                if (is_drive_word(code)) {
                    const signed long long int refused = drive_word_refused(code, given, why);
                    if (refused != success)
                        return refused;
                }
                // satellite.time's take one each, and sleep(n, unit) -- 003's spelling -- is told to
                // write the unit on the number (time_calls.hpp); sleep(5), a literal with no unit,
                // is told the same now, before the lines above it print.
                if (is_time_word(code)) {
                    signed long long int refused = time_word_refused(code, given, why);
                    if (refused == success)
                        refused = time_literal_refused(code, row, at + 2, why);
                    if (refused != success)
                        return refused;
                }
                // satellite.test's take nothing, and satellite.test itself is not a call (test_calls.hpp).
                if (is_test_word(code)) {
                    const signed long long int refused = test_word_refused(code, given, why);
                    if (refused != success)
                        return refused;
                }
                if (is_file_word(code) && given != file_word_arity(code)) {
                    why = file_word_takes(code) + ", and was given " + std::to_string(given) + " arguments";
                    return satl_line_not_understood;
                }
                // satellite.console.bash GIVEN A LITERAL THAT IS NOT TEXT -- bash(5), bash(1s) -- refused in the run's
                // sentence (console_calls.cpp) before anything runs (the review, 2026-10-23).
                if (code == word::code_of(1, 5, 13) && given == 1) {
                    std::size_t k = at + 2;
                    const Code literal = code_at(row, k);
                    const char *kind = literal == token::number_token        ? "a number"
                                       : literal == token::binary_token      ? "a binary"
                                       : literal == token::hexadecimal_token ? "a hex number"
                                       : literal == token::percentage_token  ? "a percentage"
                                       : literal == token::time_token        ? "a time"
                                                                             : nullptr;
                    if (kind != nullptr) {
                        const std::string written = text_at(row, k);
                        if (code_at(row, k) == token::right_parenthesis_token) {
                            why = std::string("satellite.console.bash takes the command as text -- "
                                              "satellite.console.bash(\"ls -l\") -- and was given ") +
                                  (literal == token::number_token && written.find('.') != std::string::npos ? "a float"
                                                                                                             : kind);
                            return types_do_not_meet;
                        }
                    }
                }
                if (code == word::code_of(1, 8, 2) || code == word::code_of(1, 8, 4)) {   // open and new, with a kind
                    const std::string written(word::spelling_of(code));
                    const signed long long int refused =
                        file_kind_written_wrong(row, at + 1, written.substr(0, written.find('(')), why);
                    if (refused != success)
                        return refused;
                }
                // A WINDOW WORD TAKES ITS OWN COUNT TOO -- three for new, one for
                // button -- so it is refused here rather than by the one-argument
                // rule below, which would tell a person the wrong thing.
                if (is_window_word(code) && given != window_word_arity(code)) {
                    // "1 argument", NOT "1 arguments". A switch is the first
                    // window word that takes NOTHING, so it is the first one a
                    // person can get wrong by exactly one -- and the sentence
                    // that tells them so reading like a machine wrote it is a
                    // small thing that is entirely avoidable.
                    why = window_word_takes(code) + ", and was given " + std::to_string(given) +
                          (given == 1 ? " argument" : " arguments");
                    return satl_line_not_understood;
                }
                // A WINDOW WORD'S ANSWER IS A WINDOW VALUE (window_calls.hpp), so the words called on it are a
                // window's and are judged as a name's are, before anything runs:
                // satellite.window.button("x").satellite.title("t") -- every link of the chain (NM-8). NAMED WITH
                // ITS BRACKETS, as every other word's answer is below, so the spelling it is told can run:
                // satellite.window.button(...).satellite.title (the verify of 0040).
                if (is_window_word(code) && a_method_period(code_at(row, close + 1)) &&
                    token::is_method_code(code_at(row, close + 2))) {
                    const signed long long int linked = window_link_periods(
                        row, close + 1, spelled_word.substr(0, spelled_word.find('(')) + (given == 0 ? "()" : "(...)"),
                        file_of(where), why, &where);
                    if (linked != success)
                        return linked;
                }
                if (!is_file_word(code) && !is_window_word(code) && !is_random_word(code) && !is_test_word(code) &&
                    !is_time_word(code) && !is_drive_word(code)) {
                    const std::string spelled(word::spelling_of(code));
                    // A WORD THAT TAKES ONE, GIVEN NONE (GTK-12). Since the lexer
                    // stopped answering nothing for empty brackets, `display()`
                    // lexes as display(text) with 0 arguments; it is refused
                    // here by name rather than at run time by a value that is
                    // not there. A word whose row IS `path()` takes none and is
                    // not this.
                    const bool takes_one = spelled.find('(') != std::string::npos &&
                                           spelled.find("()") == std::string::npos;
                    if (given > 1 || (given == 0 && takes_one)) {
                        why = spelled.substr(0, spelled.find('(')) + " takes one argument, and was given " +
                              std::to_string(given);
                        return satl_line_not_understood;
                    }
                    // A WORD WHOSE ONLY ROW IS `path()`, GIVEN SOMETHING: the lexer hands
                    // it that row now (bytecode_registry.cpp), so `satellite.console.home(5)`
                    // is told it takes nothing, not that satellite.console is not a call.
                    if (spelled.size() > 2 && spelled.compare(spelled.size() - 2, 2, "()") == 0 && given > 0) {
                        why = spelled + " takes nothing, and was given " + std::to_string(given) +
                              (given == 1 ? " argument" : " arguments");
                        return satl_line_not_understood;
                    }
                }
                // AND THE WORDS AFTER ITS ANSWER (the author's N of 2026-10-11, "n yes"), by what it answers where
                // that is known here -- satellite.file.new(p) a file, satellite.random.fast(1, 9) a number -- and,
                // where only the run knows, a bare word refused unless some spacesuit has a capsule of that name. A
                // window word's were judged above. A display's own .satellite.center() is the display's and is
                // stepped over: the chain after it is on the number both answer.
                std::size_t past = close + 1;
                if (!is_window_word(code) && a_method_period(code_at(row, past))) {
                    // NAMED WITH ITS BRACKETS, so the spelling it is told can run: satellite.infinity().satellite.string
                    std::string receiver =
                        spelled_word.substr(0, spelled_word.find('(')) + (given == 0 ? "()" : "(...)");
                    if (is_display_word(code) && code_at(row, past) == token::satellite_method_token &&
                        display_number_of(code_at(row, past + 1)) != 0) {
                        receiver += std::string(".satellite.") + method_spelling(code_at(row, past + 1));
                        past += 2;
                        std::size_t shut = past, none = 0;
                        if (code_at(row, past) == token::left_parenthesis_token && brackets_at(row, past, shut, none)) {
                            receiver += "()";
                            past = shut + 1;
                        }
                    }
                    const signed long long int after =
                        words_after_an_answer(row, past, kind_a_word_answers(code), receiver, where, why);
                    if (after != success) return after;
                }
            }
        }

        // A CONTAINER'S METHOD ON A LITERAL (the review, 2026-09-23): `", ".join({"a",
        // "b"})` -- Python's spelling -- and `3.max(5)`. A literal has no declaration
        // for method_on_a_name to judge it by, and until join and max were method
        // names this was caught by accident, as a call to a capsule nobody wrote;
        // after, only once the line ran, with output already printed. The literal's
        // kind is known right here, so it is refused before anything runs, in the
        // walker's own words. `.reverse()` is every ordered type's and is left alone.
        if (code == token::string_token || code == token::number_token || code == token::binary_token ||
            code == token::hexadecimal_token || code == token::percentage_token || code == token::time_token) {
            std::size_t past = at;
            const std::string literal = text_at(row, past);
            const Code method = code_at(row, past + 1);
            // WHAT THE LITERAL IS -- "that string", "that number" in the walker's words: 2.5 is a float. A number
            // straight after a touching slash is a fraction's bottom (fraction_values.cpp), and a method on it is
            // the fraction's: `1/3.satellite.numerator` (2026-10-11, when a fraction's words went under satellite).
            const bool decimal = code == token::number_token && literal.find('.') != std::string::npos;
            const bool a_fractions_bottom = code == token::number_token && before_this == token::fraction_token;
            const satelliteObject::Kind literal_kind = code == token::string_token       ? satelliteObject::string
                                                     : a_fractions_bottom               ? satelliteObject::fraction
                                                     : decimal                          ? satelliteObject::floating
                                                     : code == token::number_token      ? satelliteObject::number
                                                     : code == token::binary_token      ? satelliteObject::binary
                                                     : code == token::hexadecimal_token ? satelliteObject::hexadecimal
                                                     : code == token::percentage_token  ? satelliteObject::percentage
                                                                                        : satelliteObject::time;
            const std::string that = std::string("that ") + (satelliteObject::kind_name_of(literal_kind) + 2);
            // WHICH PERIOD IT TAKES (method_periods.hpp): a literal's own words are under satellite as a
            // name's of its type are -- said here in the walker's words.
            const bool under = code_at(row, past) == token::satellite_method_token;
            if (a_method_period(code_at(row, past)) && token::is_method_code(method) &&
                (number_a_method_runs(literal_kind, method) != 0) != under) {
                why = period_written_wrong(that, method_spelling(method), literal_kind, under);
                return satl_line_not_understood;
            }
            const std::string period = under ? ".satellite." : ".";
            if (a_method_period(code_at(row, past)) &&
                (method == token::foreground_token || method == token::background_token)) {
                const std::string spelled = std::string("that ") + (code == token::string_token ? "string" : "literal") +
                                            period + method_spelling(method);
                if (code != token::string_token) {
                    why = spelled + " -- a colour is given to text: \"...\".satellite." + method_spelling(method) + "(xFF8800)";
                    return types_do_not_meet;
                }
                std::size_t close = past + 2, given = 0;
                if (code_at(row, past + 2) != token::left_parenthesis_token ||
                    !brackets_at(row, past + 2, close, given) || given != 1) {
                    why = spelled + " takes one colour, in brackets";
                    return satl_line_not_understood;
                }
                why = colour_literal_refused(row, past + 3, spelled);
                if (!why.empty()) return types_do_not_meet;
            }
            // `"abc"[1]` -- [ ] STRAIGHT AFTER A STRING LITERAL is not built, and the walker says
            // so (expression.cpp, index_after_an_answer); said here, before anything runs.
            if (code == token::string_token && code_at(row, past) == token::left_square_bracket_token) {
                why = index_after_an_answer("that string");
                return satl_line_not_understood;
            }
            // A STRING'S OWN METHOD ON A STRING LITERAL (M16): judged as a name's is, by
            // string_calls.hpp, and let through when it is right -- `"a,b".split(",")`.
            if (a_method_period(code_at(row, past)) && code == token::string_token &&
                string_method_arity(method) >= 0) {
                const std::string spelled = std::string("that string") + period + method_spelling(method);
                std::size_t close = past + 2, given = 0;
                const bool bracketed = code_at(row, past + 2) == token::left_parenthesis_token;
                if (bracketed && !brackets_at(row, past + 2, close, given)) {
                    why = spelled + "( is never closed on its line";
                    return satl_line_not_understood;
                }
                const signed long long int judged =
                    string_method_check(row, past + 2, bracketed, given, method, spelled, why);
                if (judged != success)
                    return judged;
            } else if (a_method_period(code_at(row, past)) && literal_kind == satelliteObject::number &&
                       number_number_of(method) != 0) {
                // A NUMBER'S OWN METHOD ON A NUMBER LITERAL (NM-8, 2026-10-11): judged as a name's is, by
                // number_calls.hpp, and let through when it is right -- `3.satellite.max(5)`, which the line below
                // refused as a container's word while a number had none of its own.
                const std::string spelled = std::string("that number") + period + method_spelling(method);
                std::size_t close = past + 2, given = 0;
                const bool bracketed = code_at(row, past + 2) == token::left_parenthesis_token;
                if (bracketed && !brackets_at(row, past + 2, close, given)) {
                    why = spelled + "( is never closed on its line";
                    return satl_line_not_understood;
                }
                const signed long long int judged = number_method_check(bracketed, given, method, spelled, why);
                if (judged != success)
                    return judged;
            } else if (a_method_period(code_at(row, past)) &&
                       (container_arity(method) >= 0 ||
                        (string_method_arity(method) >= 0 && method != token::find_token)) &&
                       method != token::reverse_token) {
                const char *kind = code == token::string_token   ? "a string"
                                   : code == token::number_token ? "a number"
                                   : code == token::binary_token ? "a binary"
                                   : code == token::hexadecimal_token ? "a hex"
                                   : code == token::time_token        ? "a time"
                                                                      : "a percentage";
                const char *bare = kind + 2;    // "string" out of "a string"
                why = std::string("that ") + bare + period + method_spelling(method) + " is not built for " + kind +
                      " yet -- " + so_far_whose(method);
                return not_built_yet;
            }
            // AND THE REST OF ITS CHAIN, while what each link answers is known (chain_periods).
            if (a_method_period(code_at(row, past)) && token::is_method_code(method)) {
                const signed long long int chained =
                    chain_periods(row, past, literal_kind, that, file_of(where), why, &where);
                if (chained != success)
                    return chained;
            }
        }
        // A VALUE WORD AND THE WORD AFTER IT (the author's G of 2026-10-11): satellite.bool.true, a setting read by its
        // bare name, the console's width and a fact about the machine answer a value with no brackets, and the walker
        // reads a word after one as after a literal (expression.cpp, maybe_a_method) -- so its period is judged here,
        // by the kind it answers, before anything runs, and the rest of its chain with it. Before that day no
        // spelling after one could be read, and the bare one was told to mind its math signs.
        if (word::is_word_code(code) && code_at(row, at + 1) != token::left_parenthesis_token &&
            a_method_period(code_at(row, at + 1)) && token::is_method_code(code_at(row, at + 2))) {
            const satelliteObject::Kind answers = kind_a_value_word_answers(code, functions);
            if (answers != satelliteObject::how_many_kinds) {
                const std::string spelled = word::spelling_of(code);
                const Code method = code_at(row, at + 2);
                const bool under = code_at(row, at + 1) == token::satellite_method_token;
                if ((number_a_method_runs(answers, method) != 0) != under) {
                    why = period_written_wrong(spelled, method_as_written(row, at + 2, file_of(where),
                                                                         method_spelling(method)),
                                               answers, under);
                    return satl_line_not_understood;
                }
                const signed long long int chained =
                    chain_periods(row, at + 1, answers, spelled, file_of(where), why, &where);
                if (chained != success)
                    return chained;
            }
        }
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        ++at;
    }
    return success;
}

// AN OBJECT BEING DECLARED (2026-09-22): `run_log log`, `tagged_report.run_log log(path)`,
// `run_log also = log`. `at` is on the first name and left past the statement.
//
// The spacesuit must be one this line can reach; the name may not be a file's, a space's
// or a field's; what follows the name is `=` and a value, the constructor's arguments in
// brackets, or nothing -- and those arguments are counted against the constructor before
// anything runs (003 counted them only running, its S0722).
//
// DECLARED AGAIN IS A NEW OBJECT, not a second declaration to refuse, when it is the
// same spacesuit: 003's DESIGN 7.4 ("a redeclaration therefore takes a fresh slot"),
// and the author's declaration files do it 14,704 times.
signed long long int check_object_declaration(const std::vector<std::bitset<16>> &row, std::size_t &at,
                                              Where &where, DeclaredNames &declared, std::string &why)
{
    const std::size_t stop = past_the_statement(row, at);
    std::size_t k = at;
    std::vector<std::string> names;
    dotted_names_at(row, k, names);
    const std::string name = text_at(row, k);
    std::string written;
    for (const std::string &each : names) written += (written.empty() ? "" : ".") + each;

    const std::size_t suit = where.capsules.suit_named(where.scope, names, why);
    if (suit == kNoScope) { at = stop; return name_not_declared; }
    {
        const signed long long int named = a_name_it_may_take(where.capsules, where.scope, name, why);
        if (named != success) { at = stop; return named; }
    }
    if (deleted_in_another_block(where, name)) {
        why = declared_twice(where, name);
        at = stop;
        return name_declared_twice;
    }
    const DeclaredNames::const_iterator already = declared.find(name);
    if (already != declared.end()) {
        const DeclaredObjects::const_iterator object = where.objects.find(name);
        const bool a_field = where.site != nullptr && where.site->suit != kNoScope &&
                             where.capsules.scopes[where.site->suit].layout->slot_of(name) != kNoSlot;
        if (a_field || object == where.objects.end() || object->second != suit) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
    }
    const CapsuleScope &of = where.capsules.scopes[suit];
    const std::string shown = of.layout->shown;

    if (code_at(row, k) == token::assign_token) {
        const signed long long int held = names_in_statement(row, k + 1, stop, declared, where, why);
        if (held != success) { at = stop; return held; }
    } else {
        const Code after = code_at(row, k);
        const bool bracketed = after == token::left_parenthesis_token;
        if (!bracketed && after != token::line_end_token && after != token::comment_token &&
            after != token::end_of_file_token) {
            why = name + " is followed by something that is not =, its arguments in brackets, or the line's end";
            at = stop;
            return satl_line_not_understood;
        }
        std::size_t given = 0, close = k;
        if (bracketed) {
            if (!brackets_at(row, k, close, given)) {
                why = written + " " + name + "( is never closed on its line";
                at = stop;
                return satl_line_not_understood;
            }
            const Code end = code_at(row, close + 1);
            if (end != token::line_end_token && end != token::comment_token && end != token::end_of_file_token) {
                why = written + " " + name + "(...) is the whole statement, and something follows its )";
                at = stop;
                return satl_line_not_understood;
            }
        }
        // A FIELD WITH NOTHING AFTER ITS NAME IS AN EMPTY SLOT (suit_run.hpp): no
        // constructor runs, so there is nothing to count.
        if (!(where.field && !bracketed)) {
            const std::size_t receiving = where.capsules.constructor_of(suit);
            if (receiving == kNoSite) {
                if (given != 0) {
                    why = written + " " + name + " is declared with arguments, and " + shown +
                          " has no satellite.constructor to take them";
                    at = stop;
                    return satl_line_not_understood;
                }
            } else {
                const CapsuleSite &constructor = where.capsules.sites[receiving];
                if (given != constructor.parameters.size()) {
                    const std::size_t takes = constructor.parameters.size();
                    why = constructor.shown + " takes " + std::to_string(takes) +
                          (takes == 1 ? " argument" : " arguments") + ", and " + name + " was given " +
                          std::to_string(given) + (bracketed ? "" : " -- they go in brackets after the name");
                    at = stop;
                    return satl_line_not_understood;
                }
            }
        }
        if (bracketed) {
            const signed long long int held = names_in_statement(row, k + 1, close, declared, where, why);
            if (held != success) { at = stop; return held; }
        }
    }
    // AN OBJECT OF THE SAME SPACESUIT MADE AGAIN IS THE SAME LIVE NAME, and keeps the block it was first declared
    // in -- made again further in, it does not end at that block's } (the fresh reader, 2026-10-06).
    if (declared.count(name) == 0)
        where.declared_in[name] = where.this_block();
    declared[name] = word::code_of(1, 10);   // and into its place in satellite.legal (LegalNames)
    where.deleted_in.erase(name);
    where.deleted_on_line.erase(name);
    where.ended_at.erase(name);
    where.objects[name] = suit;
    where.lists.erase(name);
    where.multiples.erase(name);
    at = stop;
    return success;
}

// One statement, judged without running it. `at` is left on the code after it.
signed long long int judge_a_statement(const std::vector<std::bitset<16>> &row,
                                       std::size_t &at,
                                       Where &where,
                                       DeclaredNames &declared,
                                       EndingNames &ending,
                                       std::string &why)
{
    const CapsuleTable &capsules = where.capsules;
    const std::size_t scope = where.scope;
    const FunctionTable &functions = where.functions;
    where.statement = at;
    forget_the_finished(ending, at, declared);
    const Code code = code_at(row, at);

    if (code == token::line_end_token || code == token::left_brace_token ||
        code == token::right_brace_token) {
        ++at;
        return success;
    }

    // satellite.error() RIGHT AFTER AN if's, else's, while's OR for's } (ERROR_BLOCK_PLAN.md), with only blank lines
    // and comments between: the body loop marks each such } as it closes it. An error block's own } is never one, so
    // two in a row are refused -- and so is one after a list's }, which no body loop closes.
    if (code == word::code_of(1, 37)) {
        // AT THE PROMPT A BLOCK RUNS AS SOON AS ITS } IS TYPED, so there is nothing left for one to catch.
        if (where.typed_line) {
            why = "satellite.error() is written in a program's file, after a block's } -- at the prompt a block runs "
                  "as soon as its } is typed, so nothing is left for it to catch";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t body = error_block_body(row, at);
        if (body == kNoErrorBlock) {
            why = kErrorWrittenWrong;
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        std::size_t back = at;
        while (back > 0 && (code_at(row, back - 1) == token::line_end_token ||
                            code_at(row, back - 1) == token::comment_token))
            --back;
        if (back == 0 || back - 1 != where.block_closed_at) {
            why = kErrorWithNoBlock;
            at = past_matching_brace(row, body - 1);
            return satl_line_not_understood;
        }
        at = body - 1;              // left ON the brace, as if and while are
        where.body_brace = at;
        where.error_brace = at;
        return success;
    }

    // A LINE THAT BEGINS WITH A SIGN, refused before anything runs (the author's 6 of 2026-10-11, "refuse yes"):
    // the walker steps over a token it has no arm for, so `-n = 9` ran as n = 9 and nobody was told (ERRORS4 003).
    // A minus belongs to the value, after the =; a lone line of the shape name = number is told its spelling.
    if (code == token::tight_minus_token || code == token::minus_token || code == token::tight_plus_token ||
        code == token::plus_token) {
        const bool minus = code == token::tight_minus_token || code == token::minus_token;
        const std::string sign = minus ? "-" : "+";
        why = "a line does not begin with " + sign + " -- " +
              (minus ? "a minus belongs to the value, after the =: n = -9" : "a plus is not written before a value");
        std::size_t k = at + 1;
        if (code_at(row, k) == token::name_token) {
            const std::string name = text_at(row, k);
            std::size_t v = k + 1;
            if (code_at(row, k) == token::assign_token && code_at(row, v) == token::number_token) {
                const std::string value = text_at(row, v);
                const Code after = code_at(row, v);
                if (after == token::line_end_token || after == token::comment_token || after == token::end_of_file_token)
                    why = sign + name + " = " + value + " is written " + name + " = " + (minus ? "-" : "") + value +
                          " -- a line does not begin with " + sign;
            }
        }
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }

    // satellite.delete(name), A LINE OF ITS OWN (delete_calls.hpp) -- judged before the arms below,
    // so `satellite.delete x` is never taken for a declaration of x as a satellite.delete.
    if (is_delete_word(code)) {
        const std::size_t stop = past_the_statement(row, at);
        std::size_t past = at;
        const std::string name = the_name_to_delete(row, at, past, why);
        at = stop;
        if (name.empty())
            return satl_line_not_understood;
        return judge_a_delete(where, declared, ending, row, name, why);
    }

    // satellite.return, AND WHAT IT HANDS BACK (2026-09-22). Its value is judged like any
    // expression's, and a constructor hands back nothing -- "what a constructor produces
    // is the object" (003's S0525). Any other capsule answers whatever it hands back:
    // nothing in its header says what that is (satellite.returns, taken out 2026-09-24).
    if (code == word::code_of(1, 15)) {
        const std::size_t stop = past_the_statement(row, at);
        const std::size_t value_at = return_value_at(row, at);
        if (value_at != 0) {
            if (where.site != nullptr && where.site->constructor) {
                why = where.site->shown + " hands nothing back -- what a constructor produces is the object, so its "
                                          "satellite.return is written satellite.return()";
                at = stop;
                return satl_line_not_understood;
            }
            std::size_t close = at + 1, given = 0;
            if (!brackets_at(row, at + 1, close, given)) {
                why = "satellite.return( is never closed on its line";
                at = stop;
                return satl_line_not_understood;
            }
            const Code after = code_at(row, close + 1);
            if (after != token::line_end_token && after != token::comment_token && after != token::end_of_file_token) {
                why = "satellite.return(...) is the whole statement, and something follows its )";
                at = stop;
                return satl_line_not_understood;
            }
            if (given > 1) {
                why = "satellite.return(...) hands back one value, and was given " + std::to_string(given);
                at = stop;
                return satl_line_not_understood;
            }
            const signed long long int held = names_in_statement(row, value_at, close, declared, where, why);
            at = stop;
            return held;
        }
        at = stop;
        return success;
    }

    // A SPACESUIT, OR ONE OF ITS SECTIONS, WRITTEN INSIDE A CAPSULE (2026-09-22).
    // A spacesuit that comes to exist when a capsule runs is POLYMORPH M1, and what it
    // means is the author's to rule: where it lives once the capsule has run, and what
    // a second run makes (M1's D1 and D2).
    if (code == word::code_of(1, 10)) {
        why = "a satellite.spacesuit declared inside a capsule, that comes to exist when the capsule runs, is not "
              "built yet (POLYMORPH M1) -- declare it at the top of a file, in a satellite.namespace, or inside "
              "another spacesuit";
        at = past_the_statement(row, at);
        return not_built_yet;
    }
    if (code == word::code_of(1, 11) || code == word::code_of(1, 12) || code == word::code_of(1, 24)) {
        why = std::string(word::spelling_of(code)) + " is a section of a satellite.spacesuit, and goes inside one "
                                                     "beside the others -- not inside a capsule";
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }

    // satellite.statement.if -- the same shape as while below, judged the same way.
    if (code == word::code_of(1, 13, 1)) {
        const std::size_t stop = past_the_statement(row, at);
        const signed long long int held =
            names_in_statement(row, at + 1, stop, declared, where, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.if has no body";
            at = stop;
            return satl_line_not_understood;
        }
        at = brace;                 // left ON the brace, as while is: the loop counts it
        where.body_brace = brace;
        where.if_brace = brace;     // the } it closes with is one an else may follow (ERRORS4 027)
        return success;
    }

    // satellite.statement.else, which has no condition of its own. A REAL ONE
    // FOLLOWS AN if's `}` -- looking back one code refuses one standing alone here,
    // where nothing has run yet, instead of at the moment the walker reaches it.
    // A COMMENT IS STEPPED OVER ON THE WAY BACK (ERRORS4 016): `} // end if`, or a comment
    // line before the else, hid the if. AND A `}` THE BODY LOOP CLOSED MUST BE AN if's
    // (ERRORS4 027): a while's, a for's or an else's `}` slipped through this and was refused
    // only when it ran, in the same sentence. At the prompt, where no body loop marks them,
    // any `}` is let through as before.
    if (code == word::code_of(1, 13, 4)) {
        std::size_t back = at;
        while (back > 0 && (code_at(row, back - 1) == token::line_end_token ||
                            code_at(row, back - 1) == token::comment_token))
            --back;
        if (back == 0 || code_at(row, back - 1) != token::right_brace_token ||
            (back - 1 == where.block_closed_at && back - 1 != where.if_closed_at)) {
            why = "satellite.statement.else with no satellite.statement.if before it";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        if (back - 1 == where.error_closed_at) {
            why = "satellite.statement.else goes right after its if's } -- this one follows a satellite.error() "
                  "block, which goes after the else";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t after_else = brace_after(row, at + 1);
        if (code_at(row, after_else) == word::code_of(1, 13, 1)) {
            at = after_else;        // else written onto another if: that if is the statement
            return success;
        }
        if (code_at(row, after_else) != token::left_brace_token) {
            why = "satellite.statement.else has no body";
            at = at + 1;
            return satl_line_not_understood;
        }
        at = after_else;
        where.body_brace = after_else;
        return success;
    }

    if (code == word::code_of(1, 13, 3)) {               // satellite.statement.while
        const std::size_t stop = past_the_statement(row, at);
        const signed long long int held =
            names_in_statement(row, at + 1, stop, declared, where, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.while has no body";
            at = stop;
            return satl_line_not_understood;
        }
        // Left ON the brace, not past it: check_program's own loop counts braces,
        // and stepping over this one would make the body's `}` read as the
        // capsule's and end the check early.
        at = brace;
        where.body_brace = brace;
        where.opening_a_loop = true;
        return success;
    }

    // satellite.statement.break AND .continue (MILESTONES M20.E, M20.F): alone on their line, with
    // or without (), and inside a while or a for of this body -- an if around one is fine, a
    // capsule is not: a break cannot leave the loop of the capsule that called this one.
    if (code == word::code_of(1, 13, 5) || code == word::code_of(1, 13, 6)) {
        const std::size_t stop = past_the_statement(row, at);
        const std::string spelled(word::spelling_of(code));
        std::size_t k = at + 1;
        if (code_at(row, k) == token::left_parenthesis_token && code_at(row, k + 1) == token::right_parenthesis_token)
            k += 2;
        const Code after = code_at(row, k);
        if (after != token::line_end_token && after != token::comment_token && after != token::end_of_file_token) {
            why = spelled + " stands alone on its line: it takes nothing and nothing follows it";
            at = stop;
            return satl_line_not_understood;
        }
        if (!where.inside_a_loop()) {
            why = spelled + (code == word::code_of(1, 13, 5)
                                 ? " leaves the nearest satellite.statement.while or satellite.statement.for, "
                                 : " starts the next pass of the nearest satellite.statement.while or "
                                   "satellite.statement.for, ") +
                  "and this line is inside neither";
            at = stop;
            return satl_line_not_understood;
        }
        at = stop;
        return success;
    }

    // satellite.statement.for -- the only statement with three parts, and the
    // only one that DECLARES in its own header (MILESTONES M20.A). Its shape is
    // knowable without running and every piece of it is judged here: the two
    // semicolons, a satellite.variable.number with a name and a value, a
    // condition that is not empty, and a body.
    if (code == word::code_of(1, 13, 2)) {
        const std::size_t stop = past_the_statement(row, at);
        const ForHeader parts = for_header(row, at);
        if (!parts.ok) {
            why = "satellite.statement.for is written (satellite.variable.number <name> = <value>; "
                  "<condition>; <step>), with both semicolons";
            at = stop;
            return satl_line_not_understood;
        }
        // "you must declare a number here" (the author, M20.A). Not a string and
        // not a name already declared elsewhere: the header owns this one.
        std::size_t k = parts.declaration;
        if (code_at(row, k) == word::code_of(1, 6, 4) && code_at(row, k + 1) == word::kFirst) {
            why = kSatelliteIsImmutable;   // satellite_legal.hpp: not even a for's own number
            at = stop;
            return name_declared_twice;
        }
        if (code_at(row, k) != word::code_of(1, 6, 4) || code_at(row, k + 1) != token::name_token) {
            why = "satellite.statement.for begins with satellite.variable.number <name> = <value>";
            at = stop;
            return satl_line_not_understood;
        }
        ++k;
        const std::string name = text_at(row, k);
        if (code_at(row, k) != token::assign_token) {
            why = "satellite.statement.for's " + name + " needs a value: satellite.variable.number " + name + " = 0";
            at = stop;
            return satl_line_not_understood;
        }
        // THE VALUE IS READ BEFORE THE NAME IS DECLARED, so `for(number i = i; ...)`
        // is the same "no satellite.variable line" it would be anywhere else.
        signed long long int held =
            names_in_statement(row, k, parts.condition - 1, declared, where, why);
        if (held != success) { at = stop; return held; }
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (deleted_in_another_block(where, name) || !declared.emplace(name, word::code_of(1, 6, 4)).second) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
        where.deleted_in.erase(name);
        where.deleted_on_line.erase(name);
        where.ended_at.erase(name);
        // THE STEP IS OPTIONAL AND THE CONDITION IS NOT: M20.A gives the middle
        // part no choice ("a place to declare a condition that evaluates to true
        // or to false") and marks only the third "optionally".
        if (parts.condition == parts.step - 1) {
            why = "satellite.statement.for has nothing between its semicolons, and it needs a condition there";
            at = stop;
            return satl_line_not_understood;
        }
        where.for_step = parts.step;
        held = names_in_statement(row, parts.condition, parts.closing, declared, where, why);
        where.for_step = kNowhere;
        if (held != success) { at = stop; return held; }
        // THE STEP'S SHAPE, which is `i++`, `i--` or a math operation and nothing
        // else. The move itself is a run-time fact; which of the three it is, is
        // not, so it is refused here rather than after the loop's first turn has
        // printed (program_walk.cpp, for_step_moves_by).
        int moves_by = 0;
        held = for_step_moves_by(row, parts, name, moves_by, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.for has no body";
            at = stop;
            return satl_line_not_understood;
        }
        ending.push_back({past_matching_brace(row, brace), name});
        at = brace;                 // ON the brace, as if and while are
        where.body_brace = brace;
        where.opening_a_loop = true;
        return success;
    }

    // A CAPSULE OR A SPACE DECLARED INSIDE A CAPSULE (2026-09-22). capsules_in steps
    // over every capsule's body whole, so neither is declared there -- and without
    // these two they were told "is a declaration, and only ... are built yet", which
    // is about variables. A space that comes to exist when its capsule runs is the
    // author's idea (POLYMORPH M1) and is not built; a capsule inside a capsule has
    // never been one.
    if (code == word::code_of(1, 2)) {                   // satellite.capsule
        why = "satellite.capsule goes at the top of a file or inside a satellite.namespace, not inside another "
              "capsule";
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }
    if (code == word::code_of(1, 28)) {                  // satellite.namespace, and satellite.space
        why = "satellite.namespace goes at the top of a file or inside another satellite.namespace -- a space "
              "declared inside a capsule, that comes to exist when the capsule runs, is not built yet";
        at = past_the_statement(row, at);
        return not_built_yet;
    }

    // A DECLARATION WITH TYPES BETWEEN < AND > -- read here so the checker
    // refuses a malformed one BEFORE the program prints anything, which is the
    // whole point of the checker. read_type_shape is the walker's own parser, so
    // the two cannot come to disagree about what `<a, b>` means.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::less_than_token) {
        const std::size_t stop = past_the_statement(row, at);
        std::size_t k = at;
        TypeShape shape;
        unsigned int pending = 0;
        if (!read_type_shape(row, k, shape, pending, why) || pending != 0) {
            if (pending != 0)
                why = "there is a > here with nothing left for it to close";
            at = stop;
            return satl_line_not_understood;
        }
        if (code_at(row, k) == word::kFirst) {   // satellite, which nothing may be named (satellite_legal.hpp)
            why = kSatelliteIsImmutable;
            at = stop;
            return name_declared_twice;
        }
        if (code_at(row, k) != token::name_token) {
            why = std::string(word::spelling_of(code)) +
                  "<...> declares a name, and there is no name after the >";
            at = stop;
            return satl_line_not_understood;
        }
        // `satellite.container.list<data_unit>` NAMES A SPACESUIT, which must be one this
        // line can reach (2026-09-22).
        if (names_a_suit(shape) && !resolve_shape(capsules, scope, shape, why)) {
            at = stop;
            return name_not_declared;
        }
        const std::string name = text_at(row, k);
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (!declares(where, declared, name, code)) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        remember_shape(where, name, shape);
        const signed long long int held = names_in_statement(row, k, stop, declared, where, why);
        at = stop;
        return held;
    }

    // satellite IS THE FIRST LEGAL OBJECT OF EVERY PLACE, AND IMMUTABLE (satellite_legal.hpp): a word followed by
    // satellite alone declares the one name nothing may take -- refused for that, and not as a line with no
    // meaning, which is what it was told before satellite.legal.
    if (word::is_word_code(code) && code_at(row, at + 1) == word::kFirst &&
        code_at(row, at + 2) != token::left_parenthesis_token) {
        why = kSatelliteIsImmutable;
        at = past_the_statement(row, at);
        return name_declared_twice;
    }

    // A DECLARATION IS A WORD FOLLOWED BY A NAME. Only the type that is finished
    // is accepted: satellite.variable.number is built and checked against Python
    // across 477,253 cases, while satellite.variable.string's 23 method
    // libraries still run on the old 32-bit string (PROGRESS §5). Saying so is
    // the honest refusal; accepting it would be a declaration that does nothing.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::name_token) {
        std::size_t k = at + 1;
        const std::string name = text_at(row, k);
        const std::size_t stop = past_the_statement(row, at);
        // satellite.variable.number (1 6 4) and satellite.variable.string (1 6 1):
        // both types the object model carries as arms and both checked against
        // Python. The string joined the list on 2026-09-16, when satelliteObject
        // made satellite_string the interpreter's own string -- before that a
        // declaration of one would have been a declaration that did nothing.
        // satellite.variable.binary (1 6 5) joined the same day, as the arm
        // satellite_binary_number.
        // satellite.variable.file (1 6 2) and satellite.variable.bool (1 6 6) joined on
        // 2026-09-18 with the file type (SATELLITE_FILE_OPERATIONS FO-1, FO-2): most
        // of a file's words answer true or false, and a program has to keep them.
        // WHICH WORDS DECLARE A TYPE IS type_shape.hpp's LIST, and asking it is
        // the point rather than the tidiness. This was a chain of `code != this
        // && code != that`, and adding satellite.container.index to the language
        // did not add it here -- so `satellite.container.index s` was refused as
        // "not built yet" while the very same declaration WITH <> worked, which
        // is the sort of contradiction a hand-kept second list always grows.
        if (!is_a_type_word(code)) {
            why = std::string(word::spelling_of(code)) + " " + name +
                  " is a declaration, and only satellite.variable.number, .string, .binary, "
                  ".percentage, .file, .bool, .infinity, .float, .hex, .color, .fraction, .time, .window, "
                  ".thread, .program, .bash and satellite.container.list, .map (or .index) and .multiple are built yet";
            at = stop;
            return satl_line_not_understood;
        }
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (!declares(where, declared, name, code)) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        remember_shape(where, name, plain_shape(code));
        if (code == word::code_of(1, 6, 5) && code_at(row, k) == token::assign_token) {
            const signed long long int written = binary_is_written_with_b(row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (code == word::code_of(1, 6, 16) && code_at(row, k) == token::assign_token) {
            const signed long long int written = percentage_is_written_with_percent(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        if (code == word::code_of(1, 6, 3) && code_at(row, k) == token::assign_token) {
            const signed long long int written = time_is_written_with_its_unit(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        if (one_of_the_four(code) && code_at(row, k) == token::assign_token) {
            const signed long long int written = written_right_for(code, row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (code_at(row, k) == token::assign_token) {
            const signed long long int fits = literal_fits_the_name(code, name, row, k + 1, why);
            if (fits != success) { at = stop; return fits; }
            // A FRACTION WHERE NONE CAN GO (fraction_check.hpp): `number n = a / b`, his 9b of 2026-10-12.
            const signed long long int whole = fraction_given_where_none_goes(code, name, row, k + 1, declared, why);
            if (whole != success) { at = stop; return whole; }
        }
        const signed long long int held = names_in_statement(row, k, stop, declared, where, why);
        at = stop;
        return held;
    }

    // A LINE THAT STARTS WITH A satellite.library VALUE (library_values.hpp): writing one is
    // S250, and a line that only reads one does nothing -- both refused before anything runs.
    if (is_library_word(code) && code_at(row, at + 1) == token::method_token)
        return library_statement(capsules, where.registry, row, at, why);

    // satellite.arguments IS THE ONE WORD THAT IS NOT THIS: the run's own arguments, judged below as the bare name
    // `arguments` was until 2026-10-11 (main_arguments.hpp).
    if (word::is_word_code(code) && code != kTheRunsArgumentsWord) {
        // A WORD FOLLOWED BY `=` IS A SETTING BEING WRITTEN -- the third shape a
        // statement can start with, checked here so the walker's arm for it is
        // ever reached. The checker runs first and refuses what it has no shape
        // for, so a shape added to program_walk.cpp and not to this file is a
        // shape no program can get to.
        //
        // THE LIBRARY DECIDES WHETHER THE WORD IS A SETTING, the same way
        // expression.cpp decides it: `flag_setting` filled in. A word with `=`
        // after it and no setting library falls through to the refusal below and
        // is told it is not a call -- which is true, and is what it was told
        // before settings existed.
        if (code_at(row, at + 1) == token::assign_token) {
            const NumberRow *library = functions[code];
            if (library != nullptr && library->scenarios.flag_setting != nullptr) {
                const std::size_t stop = past_the_statement(row, at);
                // satellite.library.arguments.access = satellite.bool.false -- ITS VALUE IS JUDGED AS ANY VALUE IS,
                // so a bare true or false is refused here as everywhere, naming satellite.bool.false (the author,
                // 2026-10-11: "So there should be no true then only an error message, and satellite.bool.true" --
                // which ends MS-2's D22 of 2026-10-03, where the arguments took the bare word).
                const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
                at = stop;
                return held;
            }
        }

        // satellite.library.arguments.access(false) -- A SETTING'S FULL NAME, WRITTEN AS A CALL (the fresh reader,
        // 2026-10-06: told "false has no satellite.variable line"): how a line at the top of a file may spell it,
        // and in a capsule and at the prompt the setting is changed as satellite.arguments.access(value).
        if (code_at(row, at + 1) == token::left_parenthesis_token && a_word_of_the_arguments(code)) {
            const std::string spelled = word::spelling_of(code);
            const std::string key = spelled.substr(spelled.rfind("arguments.") + std::string("arguments.").size());
            if (const Setting *setting = a_setting_named(key)) {
                why = where.at_the_prompt || setting->read == SettingRead::while_it_runs
                          ? spelled + " is how a line at the top of a file may spell it -- in a capsule, and at the "
                                      "prompt, the setting is changed as satellite.arguments." + key + "(value)"
                          : why_a_file_line_is_too_late(*setting, spelled, false);
                at = past_the_statement(row, at);
                return word_takes_no_assignment;
            }
        }

        // A WORD NOT FOLLOWED BY ( IS NOT A CALL, and with a name after it, it
        // was a declaration above. Anything else has no shape yet.
        if (code_at(row, at + 1) != token::left_parenthesis_token) {
            // satellite, WRITTEN AS IF IT WERE A NAME -- `satellite = 5` -- is never changed (satellite_legal.hpp).
            if (code == word::kFirst) {
                why = kSatelliteIsImmutable;
                at = past_the_statement(row, at);
                return satl_line_not_understood;
            }
            // A TYPE WITH NO NAME AFTER IT is a declaration whose name did not lex as one
            // -- `satellite.variable.number !@#$ = 5`, or `9lives` -- and "is not a call"
            // told a person nothing about the name, which is what is wrong (M5).
            why = is_a_type_word(code) ? std::string(word::spelling_of(code)) + " is followed by something that is "
                                             "not a name -- " + kNameRule
                                       : std::string(word::spelling_of(code)) +
                                             " is not a call, and there is no scenario for it yet";
            // A WORD, A DOT AND A NAME IS A SPELLING THE TABLE DOES NOT HAVE: satellite.console.dispaly
            // lexes as satellite.console and then .dispaly. Said as that, with the nearest word.
            if (!is_a_type_word(code) && code_at(row, at + 1) == token::method_token &&
                code_at(row, at + 2) == token::name_token) {
                std::size_t k = at + 2;
                const std::string typed = text_at(row, k);
                // satellite.library.arguments.missing = false IN A CAPSULE: the program's switch,
                // said at the top of its file before anything runs (MS-2), and too late here. ANY OTHER
                // SETTING BY ITS FULL NAME (2026-10-06) is that spelling of a file's top line, and in a
                // capsule or at the prompt it is changed as arguments.<row>(value) (setting_writes.hpp).
                if (code == word::code_of(1, 14, 3) || code == word::code_of(1, 14, 1, 1)) {
                    std::string key;
                    std::size_t open = at + 1;
                    if (!an_argument_call(row, at + 1, key, open) || a_setting_named(key) == nullptr)
                        past_the_argument_names(row, at + 1, key);      // the = spelling's names
                    if (const Setting *setting = a_setting_named(key)) {
                        const std::string written = std::string(word::spelling_of(code)) + "." + key;
                        why = where.at_the_prompt || setting->read == SettingRead::while_it_runs
                                  ? written + " is how a line at the top of a file may spell it -- in a capsule, and "
                                              "at the prompt, the setting is changed as satellite.arguments." + key +
                                                  "(value)"
                                  : why_a_file_line_is_too_late(*setting, written, false);
                        at = past_the_statement(row, at);
                        return word_takes_no_assignment;
                    }
                }
                const std::string meant = word_it_most_likely_meant(word::spelling_of(code), typed);
                why = std::string(word::spelling_of(code)) + " has no word named " + typed +
                      (meant.empty() ? std::string() : " -- did you mean " + meant + "?");
            }
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t stop = past_the_statement(row, at);
        // A WORD'S CALL MAY BE CALLED ON (`satellite.file.open("t.se").append("x")`)
        // and must then reach the line's end, as a name's method call must.
        std::size_t close = at + 1, count = 0;
        if (brackets_at(row, at + 1, close, count)) {
            const signed long long int shaped =
                a_call_to_its_end(row, close + 1, std::string(word::spelling_of(code)) + "(...)", why);
            if (shaped != success) { at = stop; return shaped; }
        }
        const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
        at = stop;
        return held;
    }

    // AN OBJECT BEING DECLARED -- a spacesuit's name and then a name (suit_run.hpp).
    // A SPACESUIT'S NAME AND THEN satellite: no object may be named satellite either (satellite_legal.hpp).
    if (code == token::name_token) {
        std::size_t k = at;
        text_at(row, k);
        if (code_at(row, k) == word::kFirst && code_at(row, k + 1) != token::left_parenthesis_token) {
            why = kSatelliteIsImmutable;
            at = past_the_statement(row, at);
            return name_declared_twice;
        }
    }
    if (code == token::name_token && an_object_declaration_at(row, at))
        return check_object_declaration(row, at, where, declared, why);

    // A NAME -- AND satellite.arguments, CARRIED AS THE NAME kTheRunsArguments, which no variable can have.
    const bool the_runs_word = code == kTheRunsArgumentsWord;
    if (code == token::name_token || the_runs_word) {
        std::size_t k = at;
        const std::string name = the_runs_word ? std::string(kTheRunsArguments) : text_at(row, k);
        if (the_runs_word)
            ++k;                         // one code, and no letters behind it
        const std::size_t stop = past_the_statement(row, at);
        // `name(` is a capsule call; `name =` is an assignment to a declared name.
        const DeclaredNames::const_iterator found = declared.find(name);
        // A CAPSULE CALL STANDING AS ITS OWN STATEMENT -- `greet(1)`, `other.greet()`,
        // `tools.x(1)` -- the one place a capsule may be called (names_in_statement
        // says why). The walker takes `name(` as a capsule whatever else the name is,
        // and `a.b(` as one when no variable is named `a` (a_name_it_may_take keeps a
        // variable from ever sharing a file's or a space's name); this reads them the
        // same way, and hands names_in_statement only the ARGUMENTS.
        if (!the_runs_word) {
            std::vector<std::string> names;
            std::size_t open = at;
            dotted_names_at(row, open, names);
            const bool called = code_at(row, open) == token::left_parenthesis_token;
            const bool bare = called && names.size() == 1;
            const bool dotted = called && names.size() >= 2 && found == declared.end();
            if (bare || dotted) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr && (bare || reached.through_a_scope)) {
                    why = reached.why;
                    at = stop;
                    return reached.code;
                }
                if (reached.site != nullptr) {
                    std::string written = names.front();
                    for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];
                    signed long long int held = given_what_it_takes(row, open, *reached.site, written, why);
                    if (held != success) { at = stop; return held; }
                    held = a_capsule_with_an_object(*reached.site, where, written, why);
                    if (held != success) { at = stop; return held; }
                    // NOTHING AFTER ITS `)`. A call standing as a statement is walked as a
                    // statement, which steps from the `)` to the next line: `other.greet().reverse()`
                    // ran the capsule and dropped the rest without a word (the review,
                    // 2026-09-22, and as true of `greet().reverse()` before scopes).
                    std::size_t close = open, given = 0;
                    brackets_at(row, open, close, given);      // given_what_it_takes proved it closes
                    const Code after = code_at(row, close + 1);
                    if (after != token::line_end_token && after != token::comment_token &&
                        after != token::end_of_file_token) {
                        why = written + "(...) standing as a statement is the whole statement -- to use what it "
                                        "answers, declare a name with it first, as in <type> answer = " +
                              written + "(...)";
                        at = stop;
                        return satl_line_not_understood;
                    }
                    held = names_in_statement(row, open + 1, close, declared, where, why);
                    at = stop;
                    return held;
                }
            }
        }
        // THE BARE NAME THE RUN'S OWN ARGUMENTS HAD UNTIL 2026-10-11, where no name of this body is called that:
        // refused before anything runs, naming the new spelling (main_arguments.hpp).
        if (!the_runs_word && name == kTheOldBareArguments && found == declared.end() &&
            code_at(row, k) != token::left_parenthesis_token) {
            why = the_bare_arguments_refused(row, k);
            at = stop;
            return name_not_declared;
        }
        // THE ARGUMENTS, WRITTEN (main_arguments.hpp, setting_writes.hpp) -- by main's own name for them,
        // or as THE RUN'S OWN, satellite.arguments (the author, 2026-10-06, of the bare name it had until
        // 2026-10-11): `x.row = v`, and `x.row(v)` -- "the alias syntax using = ... does the same
        // thing". Judged here, before anything runs, in the words the walker asks: a setting is written
        // (for good at the prompt, for this run in a file, unless a file's line is too late for it), a
        // row satl holds is not, and a name of the program's own is main's. `argz.n += 1` is refused as
        // every name's `+=` is.
        const bool the_runs = the_runs_word;
        const bool an_arguments = the_runs || (found != declared.end() && found->second == word::code_of(1, 6, 21));
        if (an_arguments && code_at(row, k) == token::method_token) {
            ArgumentWrite write;
            write.at_the_prompt = where.at_the_prompt;
            write.the_runs = the_runs;
            std::string key;
            const std::size_t past = past_the_argument_names(row, k, key);
            const Code after = code_at(row, past);
            const bool assigned = past != k && after >= token::assign_token && after <= token::modulus_assign_token;
            std::size_t open = k;
            if (!assigned && an_argument_call(row, k, key, open)) {
                const std::string written = name + "." + key;
                why = why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions, write);
                if (!why.empty()) { at = stop; return word_takes_no_assignment; }
                std::size_t close = open, count = 0;
                if (!brackets_at(row, open, close, count) || count != 1) {
                    why = written + "(...) is given one value inside its brackets -- " + written + "(value)";
                    at = stop;
                    return satl_line_not_understood;
                }
                const Code ends = code_at(row, close + 1);
                if (ends != token::line_end_token && ends != token::comment_token && ends != token::end_of_file_token) {
                    why = written + "(...) is the whole statement -- nothing comes after its )";
                    at = stop;
                    return satl_line_not_understood;
                }
                // ITS VALUE IS JUDGED AS ANY VALUE IS: a bare true or false in the brackets is refused, naming
                // satellite.bool.false (the author, 2026-10-11, which ends MS-2's D22).
                const signed long long int held = names_in_statement(row, open + 1, close, declared, where, why);
                at = stop;
                return held;
            }
            if (assigned) {
                signed long long int held = after_the_name(row, past, name + "." + key, false, why);
                if (held == success) {
                    why = why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions, write);
                    // ITS VALUE IS JUDGED AS ANY VALUE IS: `argz.access = false` is refused, naming
                    // satellite.bool.false (the author, 2026-10-11, which ends MS-2's D22).
                    held = !why.empty() ? word_takes_no_assignment
                                        : names_in_statement(row, past + 1, stop, declared, where, why);
                }
                at = stop;
                return held;
            }
        }
        // THE RUN'S OWN, NOT WRITTEN: a line that only reads it -- `satellite.arguments.username.upper()` -- is
        // judged as the expression it is; a key in brackets is main's copy's, and the run's own rows are
        // changed by their settings, satellite.arguments.<row>(value).
        if (the_runs) {
            if (code_at(row, k) == token::left_square_bracket_token) {
                why = "satellite.arguments[...] as a line of its own writes a key, and satellite.arguments is the "
                      "run's own rows, changed only by their settings -- satellite.arguments.<row>(value); a row is "
                      "read with satellite.arguments.<row> or satellite.arguments[\"row\"]";
                at = stop;
                return satl_line_not_understood;
            }
            // `satellite.arguments.infinity[1] = 5`, `satellite.arguments.mine[1] = 5` -- AN ITEM OF A ROW: before
            // anything runs, as main's is (the fresh reader, 2026-10-06): a setting is given a value whole, a fact
            // is satl's, and a name of the program's own is main's.
            {
                std::string row_key;
                const std::size_t past = past_the_argument_names(row, k, row_key);
                std::size_t end = past, close = 0, count = 0;
                while (code_at(row, end) == token::left_square_bracket_token && brackets_at(row, end, close, count))
                    end = close + 1;
                if (past != k && end != past && code_at(row, end) == token::assign_token) {
                    ArgumentWrite write;
                    write.at_the_prompt = where.at_the_prompt;
                    write.the_runs = true;
                    why = a_setting_named(row_key) != nullptr
                              ? "satellite.arguments." + row_key + " is a setting, given a value whole -- "
                                    "satellite.arguments." + row_key + "(value) -- and there is nothing inside it "
                                    "to change"
                              : why_an_argument_is_not_written(row_key, name, nullptr, where.arguments,
                                                               where.functions, write);
                    at = stop;
                    return word_takes_no_assignment;
                }
            }
            if (!a_method_period(code_at(row, k))) {
                why = "satellite.arguments is the run's own rows, and not a variable to give a value -- a row is read "
                      "with satellite.arguments.<row>, and a setting changed with satellite.arguments.<row>(value)";
                at = stop;
                return satl_line_not_understood;
            }
            const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
            at = stop;
            return held;
        }
        if (code_at(row, k) != token::left_parenthesis_token && found == declared.end()) {
            why = where.deleted_on_line.count(name) != 0 || where.ended_at.count(name) != 0 ||
                          where.ended_earlier.count(name) != 0
                      ? not_declared_why(where, row, name)
                      : name + " has no satellite.variable line declaring it" +
                            a_name_made_elsewhere(where, name);
            at = stop;
            return name_not_declared;
        }
        // A METHOD CALL IS A STATEMENT OF ITS OWN (SATELLITE_FILE_OPERATIONS FO-1):
        // `my_file.append("line_1")` does its work and its answer is not kept. So is
        // one on a line read by number, `my_file[2].find("x")`. Everything else a
        // name can start is `=` or a capsule's `(`.
        const bool a_method_call = a_method_period(code_at(row, k)) ||
                                   code_at(row, k) == token::left_square_bracket_token;
        if (code_at(row, k) != token::left_parenthesis_token && !a_method_call) {
            const signed long long int shaped = after_the_name(row, k, name, false, why);
            if (shaped != success) { at = stop; return shaped; }
        }
        // `argz["access"] = false` AND `argz.l[1] = v` -- the rest of what main's arguments variable is
        // written by (its rows by a dot, and the call, are judged above).
        if (a_method_call && found != declared.end() && found->second == word::code_of(1, 6, 21)) {
            ArgumentWrite write;
            write.at_the_prompt = where.at_the_prompt;
            std::string key;
            const std::size_t past = past_the_argument_names(row, k, key);
            // `argz["access"] = false` -- ONE KEY IN BRACKETS. A setting by its key is judged here, before
            // anything runs, as its dotted spelling is: a sentence that says a capsule's line is too late
            // must not be said after main's lines have run (the fresh reader, 2026-10-04). Its value is judged
            // below as any value is -- a bare true or false refused, naming satellite.bool.false (the author,
            // 2026-10-11, which ends MS-2's D22).
            if (past == k && code_at(row, k) == token::left_square_bracket_token) {
                std::size_t close = 0, count = 0;
                if (brackets_at(row, k, close, count) && count == 1 && code_at(row, close + 1) == token::assign_token) {
                    std::size_t m = k + 1;
                    const std::string key = code_at(row, m) == token::string_token ? string_at(row, m) : std::string();
                    if (m == close && a_setting_named(key) != nullptr) {
                        why = why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions,
                                                             write);
                        if (!why.empty()) { at = stop; return word_takes_no_assignment; }
                    }
                }
            }
            // `argz.l[1] = v`, AN ITEM OF A ROW: allowed below as `a[i] = v` is, and refused
            // here only when the row is satl's -- which rows the program wrote, only the
            // walker knows -- or a setting, which is given a value whole.
            std::size_t end = past, close = 0, count = 0;
            while (code_at(row, end) == token::left_square_bracket_token && brackets_at(row, end, close, count))
                end = close + 1;
            if (past != k && end != past && code_at(row, end) == token::assign_token) {
                why = a_setting_named(key) != nullptr
                          ? name + "." + key + " is a setting, given a value whole -- " + name + "." + key +
                                "(value) -- and there is nothing inside it to change"
                          : why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions, write);
                if (!why.empty()) { at = stop; return word_takes_no_assignment; }
            }
        }
        // AN OBJECT'S MEMBER IS JUDGED FIRST (2026-09-22), so `log.path = x` is told that a
        // field is reached from inside its spacesuit only, and not that a call's answer
        // cannot be given a value -- true, and about the wrong thing.
        if (a_method_call && found != declared.end() && where.objects.count(name) != 0) {
            const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
            if (held != success) { at = stop; return held; }
        }
        // `s[1] = "j"` -- A STRING'S CHARACTER IS READ, NEVER WRITTEN (M16): refused in the
        // walker's own sentence (write_through_index), before anything runs.
        if (a_method_call && found != declared.end() && found->second == word::code_of(1, 6, 1) &&
            code_at(row, k) == token::left_square_bracket_token) {
            std::size_t end = k, close = 0, count = 0;
            while (code_at(row, end) == token::left_square_bracket_token && brackets_at(row, end, close, count))
                end = close + 1;
            if (code_at(row, end) == token::assign_token) {
                why = name + " is a string, and [ ] = ... changes an item of a list or a key of an index";
                at = stop;
                return types_do_not_meet;
            }
        }
        if (a_method_call && found != declared.end()) {
            const signed long long int shaped = a_call_to_its_end(row, k, name, why);
            if (shaped != success) { at = stop; return shaped; }
        }
        // The b is required on every value a binary is GIVEN, not only the first.
        if (found != declared.end() && found->second == word::code_of(1, 6, 5) &&
            code_at(row, k) == token::assign_token) {
            const signed long long int written = binary_is_written_with_b(row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (found != declared.end() && found->second == word::code_of(1, 6, 16) &&
            code_at(row, k) == token::assign_token) {
            const signed long long int written = percentage_is_written_with_percent(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        // ...and the unit on every value a time is given, not only the first.
        if (found != declared.end() && found->second == word::code_of(1, 6, 3) &&
            code_at(row, k) == token::assign_token) {
            const signed long long int written = time_is_written_with_its_unit(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        if (found != declared.end() && one_of_the_four(found->second) && code_at(row, k) == token::assign_token) {
            const signed long long int written = written_right_for(found->second, row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (found != declared.end() && code_at(row, k) == token::assign_token) {
            const signed long long int fits = literal_fits_the_name(found->second, name, row, k + 1, why);
            if (fits != success) { at = stop; return fits; }
            const signed long long int whole =
                fraction_given_where_none_goes(found->second, name, row, k + 1, declared, why);
            if (whole != success) { at = stop; return whole; }
        }
        // satellite.variable.color (2026-09-22): `c = ff00aa` IS SIX HEX DIGITS AND NOT A
        // NAME (color_check.cpp's color_names_start), so the names are looked for after it.
        std::size_t names_from = at;
        if (found != declared.end() && found->second == word::code_of(1, 6, 19) && code_at(row, k) == token::assign_token)
            names_from = color_names_start(row, k + 1, at, declared);
        const signed long long int held = names_in_statement(row, names_from, stop, declared, where, why);
        at = stop;
        return held;
    }

    // ONE CODE, NOT THE LINE: run_statements steps over only this code (its payload
    // with it) and reads the rest of the line as a statement, so the check judges that
    // same statement. Skipping the line let a stray character before a declaration hide
    // it -- a no-break space pasted as indentation made a declared n "not declared" --
    // and let `undeclared = 5` after one run past the check (the payload sweep,
    // 2026-09-17). A comment line still passes: its token steps to the line's end.
    //
    // AND ONLY THOSE TWO ARE STEPPED OVER (ERRORS4 003): any other code no statement begins with is refused. The
    // author's "refuse yes" of 2026-10-11 answered the item that named -, +, !, /* and ) together, and only the signs
    // were built (the line that begins with one, above) -- `2 n = 9`, `"x" n = 9` and `!n = 9` still ran as n = 9,
    // the lines of a /* ... */ block ran, and a line of only "text", == 3 or ) did nothing and said nothing.
    if (code != token::comment_token && code != token::error_token && code != token::end_of_file_token) {
        why = a_line_does_not_begin_with(code);
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }
    if (token::carries_a_count(code)) { text_at(row, at); return success; }
    ++at;
    return success;
}

// ---------------------------------------------------------------------------
// A SECTION'S AND A DISPLAY'S NAME IS A NAME, AND WHERE EACH MAY GO (SATELLITE_WINDOW_SECTIONS.md SEC-0)
// ---------------------------------------------------------------------------

// A DISPLAY'S SIZE AS IT WAS WRITTEN, for the sentence that says the size is a section's (rule 8): the width and the
// height between the comma after its name, `comma`, and its `)`, `close` -- "100%, 50%" -- read back from the loaded
// text of `file` as method_as_written reads a method (source_position.hpp), and "" when either cannot be: a file
// never loaded, or a size over two lines.
std::string the_size_written(const std::vector<std::bitset<16>> &row, std::size_t comma, std::size_t close,
                             const std::string &file)
{
    const auto written = [&row, &file](std::size_t from, std::size_t to) {
        std::size_t line = line_of(row, from), column = 0, to_line = line_of(row, to), to_column = 0;
        std::string text, to_text;
        if (!written_place(file, row, from, line, column, text)) {
            text = source_line(file, line);
            column = column_of(row, from, text);
        }
        if (!written_place(file, row, to, to_line, to_column, to_text))
            to_column = column_of(row, to, source_line(file, to_line));
        if (text.empty() || line != to_line || to_column <= column || to_column > text.size())
            return std::string();
        const std::string between = text.substr(column, to_column - column);
        const std::size_t first = between.find_first_not_of(" \t"), last = between.find_last_not_of(" \t");
        return first == std::string::npos ? std::string() : between.substr(first, last - first + 1);
    };
    // THE COMMA BETWEEN THE WIDTH AND THE HEIGHT, at the call's own depth: `section(1, f(2, 3))` has one more.
    std::size_t between = comma + 1, depth = 0;
    while (between < close) {
        const Code code = code_at(row, between);
        if (token::carries_a_count(code)) { skip_payload(row, between); continue; }
        if (code == token::left_parenthesis_token || code == token::left_square_bracket_token ||
            code == token::left_brace_token)
            ++depth;
        else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token ||
                 code == token::right_brace_token)
            depth = depth > 0 ? depth - 1 : 0;
        else if (code == token::comma_token && depth == 0)
            break;
        ++between;
    }
    const std::string wide = written(comma + 1, between), tall = between < close ? written(between + 1, close) : "";
    return wide.empty() || tall.empty() ? std::string() : wide + ", " + tall;
}

// WHAT A MAKER'S WORD MAKES, as a sentence names it -- "" for a code that makes nothing.
const char *what_it_makes(Code code)
{
    return code == token::window_section_token              ? "section"
           : code == token::window_display_file_token       ? "file display"
           : code == token::window_display_text_input_token ? "text input"
                                                            : "";
}

// RULE 4: THE NAMES A LINE'S SECTIONS AND DISPLAYS DECLARE, in the capsule that wrote it -- the author's design of
// 2026-10-20, "so this gives a section of the window called "section_name_here" so we can type this:
// section_name_here.satellite.window.display.file(...)", and his answer of 2026-10-25 that every display takes a
// name. Declared HERE, in check_statement, because names_in_statement is handed the names it judges as const; and
// AFTER the line is judged, as the walker declares the name once the line has made what it names
// (window_method_parts::a_piece_made) -- so a line's own arguments cannot use it. Each maker on the line with its name
// in quotes declares that name as a satellite.variable.window, with a_name_it_may_take's refusals and declared
// twice's, and two a widget's name never had: `satellite`, and the shape of a hex or binary number
// (a_widget_name_fits, asked again here for a line typed at the prompt, which check_program's pass never reads).
// It ends where any name a line declares ends: at the } of its block (close_the_block).
signed long long int the_names_a_line_makes(const std::vector<std::bitset<16>> &row, std::size_t from,
                                            std::size_t stop, Where &where, DeclaredNames &declared, std::string &why)
{
    constexpr Code kAWindow = word::fixed_code<1, 6, 18>;
    for (std::size_t at = from; at < stop && at < row.size();) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) {
            skip_payload(row, at);
            continue;
        }
        const char *made = a_method_period(code) ? what_it_makes(code_at(row, at + 1)) : "";
        if (*made == '\0' || code_at(row, at + 2) != token::left_parenthesis_token ||
            code_at(row, at + 3) != token::string_token) {
            ++at;
            continue;
        }
        std::size_t k = at + 3;
        const std::string name = text_at(row, k);
        if (!a_widget_name_fits(name, why))
            return satl_line_not_understood;
        const signed long long int may = a_name_it_may_take(where.capsules, where.scope, name, why);
        if (may != success)
            return may;
        if (!declares(where, declared, name, kAWindow)) {
            // `satellite.variable.window top = main_window.satellite.window.section("top", ...)`: the line's own
            // declaration took the name first, and the plan's sentence is the one for it.
            std::size_t named_at = from + 1;
            const bool its_own_declaration = word::is_word_code(code_at(row, from)) &&
                                             code_at(row, named_at) == token::name_token &&
                                             text_at(row, named_at) == name;
            why = its_own_declaration ? declared_by_its_line(name, made) : declared_twice(where, name, made);
            return name_declared_twice;
        }
        where.made_by[name] = made;
        remember_shape(where, name, plain_shape(kAWindow));
        at = k;
    }
    return success;
}

// A NAME THE CHECKER NO LONGER SEES WHOLE, and everything made in it: anything that could change what is in a
// window can change what is in its sections too.
void stop_seeing(Where &where, const std::string &name)
{
    std::vector<std::string> going{name};
    while (!going.empty()) {
        const std::string one = going.back();
        going.pop_back();
        where.seen_whole.erase(one);
        for (const std::pair<const std::string, Where::SeenWhole> &each : where.seen_whole)
            if (each.second.made_in == one)
                going.push_back(each.first);
    }
}

// RULES 6 AND 7, BEFORE THE RUN, WHERE THE CHECKER CAN SEE THE LINES (SEC-0): a second display in a section, a display
// in a section that holds sections, a section in a section that shows a display, a display across the top of a
// window that has sections, and a section in a window that shows a display across its top -- each said in
// window_shapes.cpp's sentence, the one window_widget_made says when the line runs.
//
// IT SEES ONLY WHAT IS SURE. A line is judged when it runs straight down the capsule (Where::straight_down), its
// receiver is a name seen whole, and the line is that name, `.satellite.`, the maker, and its name in quotes. What
// that line makes is seen from then on, empty, when nothing follows its brackets. A window is seen from
// `satellite.variable.window w = satellite.window.new(...)` straight down. And every other place a seen name stands
// in a line -- an argument, a value, a capsule's call, a block, `w = ...`, `w.satellite.close()` -- stops it being
// seen, with what was made in it: from there only the run knows what it holds. So does `w.satellite.open()`, which
// puts back a window a person closed without the sections that closed with it (window_open.cpp, window_widgets.cpp's
// "what closed with its window is not counted"; the fresh reader of SEC-0, 2026-10-25). So the checker refuses only
// a line the run would refuse too, and the run refuses the rest (window_widget_made).
signed long long int where_it_goes_refused(const std::vector<std::bitset<16>> &row, std::size_t from,
                                           std::size_t stop, Where &where, std::string &why)
{
    // ITS OWN LINE: `top.satellite.window.display.file("files")`, straight down -- or `w.satellite.close()`, `.open()`.
    std::size_t k = from;
    const bool led_by_a_name = where.straight_down && code_at(row, from) == token::name_token;
    if (led_by_a_name)
        skip_payload(row, k);
    const bool its_own = led_by_a_name && code_at(row, k) == token::satellite_method_token;
    const Code method = its_own ? code_at(row, k + 1) : Code{0};
    // EVERY OTHER NAME THE LINE WRITES, and the receiver of a line that is not its own: no longer seen. Read only
    // while something is seen, so a capsule with no window costs this nothing but the walk.
    for (std::size_t at = its_own ? k : from; !where.seen_whole.empty() && at < stop && at < row.size();) {
        if (code_at(row, at) == token::name_token) {
            const std::string name = text_at(row, at);
            stop_seeing(where, name);
            continue;
        }
        if (token::carries_a_count(code_at(row, at))) {
            skip_payload(row, at);
            continue;
        }
        ++at;
    }
    if (!where.straight_down)
        return success;
    // A WINDOW FROM satellite.window.new, given to the name its line declares, and nothing after the brackets.
    if (code_at(row, from) == word::fixed_code<1, 6, 18> && code_at(row, from + 1) == token::name_token) {
        std::size_t n = from + 1;
        const std::string name = text_at(row, n);
        const Code maker = code_at(row, n + 1);
        std::size_t close = n + 2, given = 0;
        if (code_at(row, n) == token::assign_token &&
            (maker == word::fixed_code<1, 27, 1> || maker == word::fixed_code<1, 27, 24>) &&
            brackets_at(row, n + 2, close, given) &&
            (code_at(row, close + 1) == token::line_end_token || code_at(row, close + 1) == token::comment_token ||
             code_at(row, close + 1) == token::end_of_file_token)) {
            Where::SeenWhole window;
            window.a_window = true;
            where.seen_whole[name] = window;
        }
        return success;
    }
    const char *made = what_it_makes(method);
    const bool closes_or_opens = method == token::close_token || method == token::open_token;
    if (!its_own || (!closes_or_opens && *made == '\0'))
        return success;
    std::size_t r = from;
    const std::string receiver = text_at(row, r);
    if (closes_or_opens) {
        stop_seeing(where, receiver);
        return success;
    }
    if (code_at(row, k + 2) != token::left_parenthesis_token || code_at(row, k + 3) != token::string_token)
        return success;
    std::size_t m = k + 3;
    const std::string name = text_at(row, m);
    std::size_t close = k + 2, given = 0;
    const bool ends_there = brackets_at(row, k + 2, close, given) &&
                            (code_at(row, close + 1) == token::line_end_token ||
                             code_at(row, close + 1) == token::comment_token ||
                             code_at(row, close + 1) == token::end_of_file_token);
    const bool a_section = method == token::window_section_token;
    const std::unordered_map<std::string, Where::SeenWhole>::iterator holder = where.seen_whole.find(receiver);
    if (holder != where.seen_whole.end()) {
        Where::SeenWhole &in = holder->second;
        const std::string spelled = receiver + ".satellite." + token::method_name_of(method);
        if (in.a_section || in.a_window) {
            const bool refused = a_section ? in.shows_a_display : (in.a_section && in.shows_a_display) || in.holds_sections;
            if (refused) {
                const APlace place = a_section ? (in.a_window ? APlace::a_section_under_displays
                                                              : APlace::a_section_beside_a_display)
                                     : in.a_window      ? APlace::a_display_over_sections
                                     : in.shows_a_display ? APlace::a_second_display
                                                          : APlace::a_display_among_sections;
                why = spelled + " -- " + a_place_refused(place);
                return satl_line_not_understood;
            }
            (a_section ? in.holds_sections : in.shows_a_display) = true;
        }
    }
    // WHAT THE LINE MADE IS SEEN FROM HERE, EMPTY -- made in whatever its receiver holds, seen or not -- when nothing
    // after its brackets could have changed it.
    if (ends_there) {
        Where::SeenWhole fresh;
        fresh.a_section = a_section;
        fresh.made_in = receiver;
        where.seen_whole[name] = fresh;
    }
    return success;
}

// EVERY STATEMENT, judged (judge_a_statement), and then what SEC-0 adds to it: the names its sections and displays
// declare (rule 4), and whether each may go where it was made (rules 6 and 7). `at` is left where the statement's
// judgement left it -- an if's and a while's on its brace -- so a maker in an if's header declares its name at the
// level the if stands at, where the walker declares it.
signed long long int check_statement(const std::vector<std::bitset<16>> &row,
                                     std::size_t &at,
                                     Where &where,
                                     DeclaredNames &declared,
                                     EndingNames &ending,
                                     std::string &why)
{
    const std::size_t from = at;
    const signed long long int judged = judge_a_statement(row, at, where, declared, ending, why);
    if (judged != success || at <= from)
        return judged;
    const signed long long int named = the_names_a_line_makes(row, from, at, where, declared, why);
    if (named != success)
        return named;
    return where.seen_whole.empty() && !where.straight_down ? success
                                                            : where_it_goes_refused(row, from, at, where, why);
}

} // namespace

signed long long int check_typed_line(const BytecodeRegistry &registry,
                                      const FunctionTable &functions,
                                      const TypedLineMemory &kept,
                                      MachineState &state)
{
    static const CapsuleTable none;
    EndingNames ending;
    Where where{registry, none, kNoScope, functions};
    where.typed_line = true;
    where.arguments = state.arguments;   // the rows satl holds, which a typed line may not write either
    where.at_the_prompt = state.at_the_prompt;
    where.place = &legal_place("satellite.legal.prompt");
    DeclaredNames declared(*where.place);   // the prompt's own place in satellite.legal, read from here
    where.names_end_with_their_block = state.names_end_with_their_block = names_end_at_their_block(state.arguments);
    // WHAT EARLIER LINES KEPT, read from the table the run wrote -- one source, so the
    // check can never believe in a name the run did not make. A name whose block has ended
    // is kept as a record, and is legal nowhere (satellite_legal.hpp).
    for (const std::pair<const std::string, Variable> &name : kept.variables) {
        if (!name.second.accessible) { where.ended_earlier[name.first] = true; continue; }
        declared[name.first] = name.second.declared;
        remember_shape(where, name.first, name.second.shape);
    }
    const std::vector<std::bitset<16>> &row = registry.front();
    {
        std::string why;   // a character or an escape that means nothing, as a file refuses it
        if (first_thing_with_no_meaning(row, 0, why) < row.size())
            return report_error("satl(prompt): " + why, satl_line_not_understood);
    }
    for (std::size_t at = 0; at < row.size() && code_at(row, at) != token::end_of_file_token; ) {
        // A BLOCK TYPED AT THE PROMPT (2026-09-25): its braces are stepped over as a
        // capsule body's are, and each statement inside is judged in turn.
        if (code_at(row, at) == token::left_brace_token || code_at(row, at) == token::right_brace_token) {
            std::string why;
            if (code_at(row, at) == token::left_brace_token && a_brace_that_opens_nothing(where, at, why))
                return report_error("satl(prompt): " + why, satl_line_not_understood);
            if (code_at(row, at) == token::left_brace_token) where.opened(); else close_the_block(where, declared, at);
            ++at;
            continue;
        }
        const std::size_t was = at;
        std::string why;
        if (habit_from_another_language(row, at, why))
            return report_error("satl(prompt): " + why, satl_line_not_understood);
        const signed long long int stopped = check_statement(row, at, where, declared, ending, why);
        if (stops_the_program(stopped))
            return report_error("satl(prompt): " + why, stopped);
        for (const std::pair<std::string, Code> &replaced : where.replacing)
            declared[replaced.first] = replaced.second;
        where.replacing.clear();
        if (at <= was)                  // a statement must always move forward
            ++at;
    }
    (void)state;
    return success;
}

signed long long int check_prompt_statements(const BytecodeRegistry &registry,
                                             const CapsuleTable &capsules,
                                             const CapsuleSite &site,
                                             const FunctionTable &functions,
                                             const TypedLineMemory &kept,
                                             MachineState &state)
{
    // THE SAME JUDGEMENT AS A TYPED LINE, with the prompt's own capsules and spacesuits
    // around it: the statement is the body of a hidden capsule at the end of what the
    // session declared (session.cpp), so a bare capsule name reaches a declared capsule and
    // a spacesuit's name is a type -- and the kept names are its first names, as a
    // capsule's parameters are.
    EndingNames ending;
    Where where{registry, capsules, site.scope, functions};
    where.site = &site;
    where.typed_line = true;
    where.arguments = state.arguments;
    where.at_the_prompt = state.at_the_prompt;
    where.place = &legal_place("satellite.legal.prompt");
    site.legal_place = &where.place->name;
    DeclaredNames declared(*where.place);   // the prompt's own place in satellite.legal, read from here
    where.names_end_with_their_block = state.names_end_with_their_block = names_end_at_their_block(state.arguments);
    site.names_end_with_their_block = where.names_end_with_their_block;
    // AND THE SESSION'S OWN CAPSULES, made again in this table for this statement: their places, for the names a
    // call of one declares -- and the rule each was CHECKED under when it was declared, kept on its place, whatever
    // arguments.access says now (the fresh reader, 2026-10-06).
    for (const CapsuleSite &each : capsules.sites)
        if (&each != &site) {
            LegalPlace &its = legal_place(each.shown, legal_place_name(each.shown));   // found by the capsule, called by its place
            each.legal_place = &its.name;
            each.names_end_with_their_block = its.names_end_with_their_block;
        }
    for (const std::pair<const std::string, Variable> &name : kept.variables) {
        if (!name.second.accessible) { where.ended_earlier[name.first] = true; continue; }   // a record of a name whose block ended (satellite_legal.hpp)
        declared[name.first] = name.second.declared;
        remember_shape(where, name.first, name.second.shape);
    }
    const std::vector<std::bitset<16>> &row = registry[site.row];
    std::size_t depth = 0;
    for (std::size_t at = site.body; at < row.size(); ) {
        const Code code = code_at(row, at);
        if (code == token::end_of_file_token)
            break;
        if (code == token::right_brace_token) {
            if (depth == 0) break;      // the hidden capsule's own closing brace
            --depth;
            close_the_block(where, declared, at);
            ++at;
            continue;
        }
        if (code == token::left_brace_token) {
            std::string why;
            if (a_brace_that_opens_nothing(where, at, why))
                return report_error("satl(prompt): " + why, satl_line_not_understood);
            ++depth; where.opened(); ++at; continue;
        }
        const std::size_t was = at;
        std::string why;
        if (habit_from_another_language(row, at, why))
            return report_error("satl(prompt): " + why, satl_line_not_understood);
        const signed long long int stopped = check_statement(row, at, where, declared, ending, why);
        if (stops_the_program(stopped))
            return report_error("satl(prompt): " + why, stopped);
        for (const std::pair<std::string, Code> &replaced : where.replacing)
            declared[replaced.first] = replaced.second;
        where.replacing.clear();
        if (at <= was)
            ++at;
    }
    (void)state;
    return success;
}

signed long long int check_program(const BytecodeRegistry &registry,
                                   const CapsuleTable &capsules,
                                   const FunctionTable &functions,
                                   MachineState &state)
{
    // WHAT THE SCAN FOUND COMES FIRST, AND THE EARLIEST OF IT. capsules_in() has no
    // program to stop and no sentence to print, so a header it could not read, a
    // name declared twice in one scope, a variable inside a satellite.namespace or
    // a spacesuit nobody can declare yet is refused here -- with a caret now, which
    // the header refusals never had. EARLIEST BY FILE AND POSITION, and not in the
    // order the passes happened to find them: nobody reads a program by pass.
    //
    // AND THEN THE CAPSULES IN FILE ORDER. The table was an unordered_map until
    // scopes, and which of two wrong capsules a person was told about depended on
    // which one the hash put first.
    //
    // EXCEPT A BODY THE FILE ENDS INSIDE, which is held until the bodies are checked
    // (ScopeTrouble::after_the_bodies): the statement that left a { open says it better.
    // ONLY WHEN IT IS THE ONLY KIND OF TROUBLE: any other the scan found is said first,
    // earliest of them, because the bodies must never be checked against a table the scan
    // could not finish -- a spacesuit type left unresolved crashed the checker (the review
    // of the error sweep, 2026-09-25: a missing } and a `shp s` parameter, signal 11).
    const ScopeTrouble *held = nullptr;
    if (!capsules.troubles.empty()) {
        const ScopeTrouble *first = nullptr;
        for (const ScopeTrouble &each : capsules.troubles) {
            if (each.after_the_bodies) {
                if (held == nullptr || each.row < held->row || (each.row == held->row && each.at < held->at))
                    held = &each;
                continue;
            }
            if (first == nullptr || each.row < first->row || (each.row == first->row && each.at < first->at))
                first = &each;
        }
        if (first != nullptr)
            return raise_at(first->code, first->why, std::string(), state, registry[first->row], first->at,
                            "satl(check)");
    }

    // EVERY satellite.library VALUE, READ ONCE (library_values.hpp) -- before any capsule
    // is judged, and so before anything runs. Values, not globals: nothing changes one.
    {
        const signed long long int read = read_library_values(registry, capsules, functions, state);
        if (stops_the_program(read))
            return read;
    }

    // EVERY NAME A WIDGET IS GIVEN, IN EVERY FILE (the author's .widgets, 2026-10-09):
    // `.satellite.window.display.file("file_display")` anywhere makes `my_window.file_display` a name the
    // program may write anywhere -- in main, and in the capsule a key runs, as `its_window.file_display`. THE NAME
    // IS WRITTEN IN QUOTES, so it is known before the run: a name worked out by the run could never be checked.
    // (Since SEC-0 the line also declares the name in its own capsule, to be written bare there --
    // the_names_a_line_makes; this set is the names reached through a window, in every capsule.)
    std::unordered_set<std::string> widget_names;
    {
        std::unordered_set<std::size_t> read_rows;
        for (const CapsuleSite &site : capsules.sites) {
            if (!read_rows.insert(site.row).second)
                continue;
            const std::vector<std::bitset<16>> &row = registry[site.row];
            const std::string file_of_row = state.program_files != nullptr && site.row < state.program_files->size()
                                                ? (*state.program_files)[site.row]
                                                : std::string();
            for (std::size_t at = 0; at < row.size(); ) {
                const Code code = code_at(row, at);
                // `.satellite.window.display.file(...)`, `.satellite.window.display.text.input(...)` and
                // `.satellite.window.section(...)` (SATELLITE_WINDOW_SECTIONS.md SEC-0), one word each, which the lexer
                // reads whole only after `.satellite.` -- so none of them is ever an object's capsule, and each is
                // judged here, wherever it is written, before anything runs.
                const Code made = a_method_period(code) ? code_at(row, at + 1) : Code{0};
                const bool a_display = makes_a_widget(made);
                const bool a_section = made == token::window_section_token;
                // AN OLD SPELLING -- `.section`, `.satellite.widgets.display.file`, or the display word written as its
                // parts, `.display.file("files")`, `.displays.input(...)` -- names its widget too, and is not judged
                // here: its line is refused for its spelling, naming the new one (rule 8), and a name it gave must not
                // be refused again, wherever it is used, as one nobody gave. `.section` is an object's capsule's name
                // as well, so it is never refused here.
                const bool written_as_parts = (code == token::display_token || code == token::displays_token) &&
                                              a_method_period(code_at(row, at + 1)) &&
                                              (code_at(row, at + 2) == token::file_piece_token ||
                                               code_at(row, at + 2) == token::input_piece_token);
                const bool an_old_spelling = made == token::section_token || made == token::widgets_display_file_token ||
                                             made == token::widgets_display_input_token;
                if (written_as_parts || an_old_spelling) {
                    const std::size_t open = written_as_parts ? at + 3 : at + 2;
                    std::size_t k = open + 1;
                    std::string why;
                    if (code_at(row, open) == token::left_parenthesis_token && code_at(row, k) == token::string_token) {
                        const std::string named = text_at(row, k);
                        if (a_widget_name_fits(named, why))
                            widget_names.insert(named);
                    }
                    ++at;
                    continue;
                }
                if (a_display || a_section) {
                    const std::size_t made_at = at + 1;
                    const std::string spelled = std::string(".satellite.") + token::method_name_of(made);
                    std::string named, why;
                    // A SECTION: its name in quotes, a width and a height (section_written_wrong, which the line's
                    // own check asks again with the receiver it was written on). ASKED HERE, where the sentence
                    // cannot name the receiver, because on an item, a call's answer or satellite.window.new(...)
                    // itself -- `ws[1].satellite.window.section("top", 100%)` -- no line's own check reaches it,
                    // and it would wait for the run (the fresh reader of SEC-0 and the probes after, 2026-10-25).
                    //
                    // ITS NAME AND ITS SHAPE ONLY, HERE (SEC-1's fresh reader): this pass runs over every capsule
                    // before any line is checked, so a size it refused would be told ahead of a real error on an
                    // earlier line. A size written as a literal is the line's own check's to refuse, in its turn; on
                    // an item or a call's answer, which no line's check reaches, the run refuses it.
                    if (a_section) {
                        const signed long long int judged = section_written_wrong(row, made_at + 1, spelled, why);
                        if (judged != success && judged != not_a_position && judged != types_do_not_meet)
                            return raise_at(judged, why, std::string(), state, row, made_at, "satl(check)");
                    }
                    // A DISPLAY: THE NAME IN QUOTES, AND NOTHING AFTER IT -- it fills its section (the author,
                    // 2026-10-25), so a display given a size is told the size is a section's (rule 8), as written.
                    std::size_t k = made_at + 2;
                    if (code_at(row, made_at + 1) == token::left_parenthesis_token && code_at(row, k) == token::string_token)
                        named = text_at(row, k);
                    std::size_t close = made_at + 1, given = 0;
                    const bool closed = brackets_at(row, made_at + 1, close, given);
                    if (a_display) {
                        const bool in_quotes = code_at(row, made_at + 1) == token::left_parenthesis_token &&
                                               code_at(row, made_at + 2) == token::string_token &&
                                               (code_at(row, k) == token::right_parenthesis_token ||
                                                code_at(row, k) == token::comma_token);
                        if (!in_quotes)
                            why = spelled + " takes the display's name, in quotes -- " + spelled +
                                  "(\"file_display\") -- a name worked out by the run could not be checked before it";
                        else if (closed && given > 1)
                            why = a_display_takes_no_size(made, spelled + "(\"" + named + "\", ...)", named,
                                                          given == 3 && code_at(row, k) == token::comma_token
                                                              ? the_size_written(row, k, close, file_of_row)
                                                              : std::string());
                        else
                            a_widget_name_fits(named, why);
                        if (!why.empty())
                            return raise_at(satl_line_not_understood, why, std::string(), state, row, made_at,
                                            "satl(check)");
                    }
                    widget_names.insert(named);
                }
                if (token::carries_a_count(code)) {
                    skip_payload(row, at);
                    continue;
                }
                ++at;
            }
        }
    }

    for (const CapsuleSite &site : capsules.sites) {
        const std::vector<std::bitset<16>> &row = registry[site.row];
        Where where{registry, capsules, site.scope, functions};
        where.site = &site;
        where.widget_names = &widget_names;
        where.arguments = state.arguments;
        where.at_the_prompt = state.at_the_prompt;
        // ITS PLACE IN satellite.legal, satellite first (satellite_legal.hpp) -- and every call of it points its
        // names there; and whether names end at their block, the one decision the walker reads too.
        where.place = &legal_place(site.shown, legal_place_name(site.shown));   // found by the capsule, called by its place
        site.legal_place = &where.place->name;
        where.names_end_with_their_block = state.names_end_with_their_block = names_end_at_their_block(state.arguments);
        site.names_end_with_their_block = where.place->names_end_with_their_block = where.names_end_with_their_block;
        // THE NAMES LEGAL IN THIS CAPSULE ARE ITS PLACE'S, read and written there (LegalNames). One place a
        // capsule: there are no globals, so a name declared elsewhere is not declared here.
        DeclaredNames declared(*where.place);
        // A SPACESUIT'S CAPSULE SEES ITS OBJECT'S FIELDS BY THEIR BARE NAMES (2026-09-22):
        // the author's `path = path_input`, `satellite.return(spacesuit_name)`. They are
        // its first declared names, as its parameters are, and a field that holds an
        // object is one whose members are judged against that object's spacesuit.
        // In order, so a field its spacesuit declared again over a supertype's is the one
        // the name means here.
        if (site.suit != kNoScope)
            for (const SuitField &field : capsules.scopes[site.suit].layout->fields) {
                declared[field.name] = field.shape.word;
                remember_shape(where, field.name, field.shape);
            }
        // ITS PARAMETERS ARE ITS FIRST DECLARED NAMES (2026-09-21). They are
        // declared by the header rather than by a satellite.variable line, and
        // without this the body that uses one is refused with "has no
        // satellite.variable line declaring it" -- a true sentence about a name
        // that really was declared, just not where the checker was looking.
        //
        // satellite.main's TOO, SINCE 2026-09-22. It was left out on purpose while
        // run_main bound nothing -- a declared name that would not be there moves
        // the refusal from the checker to the walker. run_main binds it now: it is
        // the arguments variable, every row satl holds (main_arguments.hpp).
        for (const CapsuleParameter &takes : site.parameters) {
            std::string why;
            const signed long long int named = a_name_it_may_take(capsules, site.scope, takes.name, why);
            if (named != success)
                return raise_at(named, why, site.shown, state, row, site.declared_at, "satl(check)");
            if (declared.count(takes.name) != 0)
                return raise_at(name_declared_twice, declared_twice(where, takes.name), site.shown, state, row,
                                site.declared_at, "satl(check)");
            // main's is the arguments variable, and is WRITTEN as one (the author's K of 2026-10-11): the older
            // header, main(satellite.container.list<satellite.variable.string> arguments), is refused by name.
            if (site.name == "satellite.main" && takes.declared() != word::code_of(1, 6, 21))
                return raise_at(satl_line_not_understood,
                                "satellite.main's parameter is written satellite.variable.arguments " + takes.name +
                                    " now -- it holds the arguments, and " + std::string(word::spelling_of(takes.declared())) +
                                    " was their older spelling",
                                site.shown, state, row, site.declared_at, "satl(check)");
            declared[takes.name] = site.name == "satellite.main" ? word::code_of(1, 6, 21) : takes.declared();
            remember_shape(where, takes.name, takes.shape);
        }
        EndingNames ending;
        // THE BODY THE FILE ENDS INSIDE is read to the file's end, or to the next thing the
        // file declares -- a capsule written after it is the file's, not a line of this one.
        const bool unclosed = held != nullptr && held->row == site.row && held->at == site.declared_at;
        // THE LAST STATEMENT OF THE BODY ITSELF, for the rule below about main's.
        std::size_t last_statement = kNoStatement, closing = row.size();
        // A BODY WITH A satellite.error() AFTER IT (ERROR_BLOCK_PLAN.md) IS JUDGED AS A BLOCK, so its names end at its
        // } as they do when it runs, and then the error block is judged the same way: it sees the parameters and the
        // fields, and nothing the body declared. last_statement and closing stay the body's own.
        const bool guarded = site.error_body != kNoErrorBlock;
        for (const std::size_t part : {site.body, site.error_body}) {
            if (part == kNoErrorBlock)
                break;
            if (guarded)
                where.opened();
            where.block_closed_at = where.error_closed_at = where.if_closed_at = kNowhere;
            // WHAT THE CHECKER SEES WHOLE (SEC-0) is the body's: an error block runs after whatever line failed.
            where.seen_whole.clear();
            std::size_t depth = 0, last_here = kNoStatement, ends = row.size();
            for (std::size_t at = part; at < row.size(); ) {
                const Code code = code_at(row, at);
                if (code == token::end_of_file_token)
                    break;
                if (unclosed && (code == word::code_of(1, 2) || code == word::code_of(1, 10) || code == word::code_of(1, 28)))
                    break;
                if (code == token::right_brace_token) {
                    if (depth == 0) { ends = at; break; }         // the capsule's own closing brace
                    --depth;
                    close_the_block(where, declared, at);         // its names end here (satellite_legal.hpp)
                    where.error_block_closed(at);
                    ++at;
                    continue;
                }
                if (code == token::left_brace_token) {
                    std::string why;
                    where.error_block_opened(at);
                    if (a_brace_that_opens_nothing(where, at, why))
                        return raise_at(satl_line_not_understood, why, site.shown, state, row, at, "satl(check)");
                    ++depth; where.opened(); ++at; continue;
                }
                const std::size_t was = at;
                // A BLANK LINE OR A COMMENT IS NOT A STATEMENT (ERRORS4 019), so neither is main's last one: one
                // between satellite.return(satellite) and main's } was S103, asking for the return already there.
                if (depth == 0 && code != token::line_end_token && code != token::comment_token) last_here = at;
                std::string why;
                if (habit_from_another_language(row, at, why))
                    return raise_at(satl_line_not_understood, why, site.shown, state, row, at, "satl(check)");
                // A LINE AT THE BODY'S OWN LEVEL RUNS STRAIGHT DOWN, once and in order (SEC-0, where_it_goes_refused).
                where.straight_down = depth == 0 && part == site.body;
                const signed long long int code_of_line = check_statement(row, at, where, declared, ending, why);
                if (stops_the_program(code_of_line))
                    // THE STATEMENT'S OWN START, AND NOT WHERE `at` ENDED UP. A
                    // refusal leaves `at` wherever check_statement stopped reading,
                    // which is not reliably the thing that was wrong -- so the caret
                    // goes under the start of the statement, which always is. The
                    // LINE is exact either way, and that is what a person looks for
                    // first.
                    return raise_at(code_of_line, why, site.shown, state, row, was, "satl(check)");
                if (at <= was)                  // a statement must always move forward
                    ++at;
            }
            if (guarded)
                close_the_block(where, declared, ends);
            if (part == site.body) {
                last_statement = last_here;
                closing = ends;
            }
        }
        // satellite.main ENDS WITH satellite.return(satellite) (the author, 2026-09-25: "let's
        // refuse a main that doesnt have satellite.return(satellite) as the last line, but still
        // allow the user to satellite.return(satellite) to quit the interpreter from anywhere").
        // The program's own main only -- an included file's is not the one being run -- and
        // its last statement at the body's own depth: a return inside a last if is a way to
        // quit early, not the last line.
        if (&site == capsules.main() && !unclosed) {
            const bool ends_right = last_statement != kNoStatement &&
                                    code_at(row, last_statement) == word::code_of(1, 15) &&
                                    code_at(row, last_statement + 1) == token::left_parenthesis_token &&
                                    code_at(row, last_statement + 2) == word::code_of(1) &&
                                    code_at(row, last_statement + 3) == token::right_parenthesis_token;
            if (!ends_right)
                return raise_at(satl_file_missing_satellite_return_satellite,
                                "satellite.main's last line is satellite.return(satellite) -- it is where the program "
                                "ends, so it goes just before main's closing }",
                                std::string(), state, row, last_statement != kNoStatement ? last_statement : closing,
                                "satl(check)");
        }
    }

    // EVERY FIELD'S OWN STATEMENT (2026-09-22), judged as the declaration it is -- each
    // alone, as 003 made them: a field's value is worked out before there is an object,
    // so it cannot name another field or call its spacesuit's capsules (003's S0511).
    for (std::size_t s = 0; s < capsules.scopes.size(); ++s) {
        const CapsuleScope &suit = capsules.scopes[s];
        if (!suit.is_a_suit())
            continue;
        const std::vector<std::bitset<16>> &row = registry[suit.row];
        // ITS OWN FIELDS: a supertype's were judged where they are declared.
        for (std::size_t slot = suit.layout->own_fields; slot < suit.layout->fields.size(); ++slot) {
            const SuitField &field = suit.layout->fields[slot];
            DeclaredNames declared;
            EndingNames ending;
            Where where{registry, capsules, s, functions};
            where.field = true;
            where.arguments = state.arguments;   // satl's rows, as the other three places have them (N, 2026-10-11)
            std::size_t at = field.at;
            std::string why;
            const signed long long int code_of_line = check_statement(row, at, where, declared, ending, why);
            if (stops_the_program(code_of_line))
                return raise_at(code_of_line, why, suit.layout->shown, state, row, field.at, "satl(check)");
        }
        // A SUPERTYPE'S CONSTRUCTOR THAT WANTS ARGUMENTS, when this spacesuit has one of its
        // own: the object's arguments go to the nearest constructor, and every one above it
        // runs with none -- 003's order, which had no super(...) to hand them over either.
        const std::size_t receiving = capsules.constructor_of(s);
        for (std::size_t n = 1; n < suit.layout->lineage.size(); ++n) {
            const std::size_t above = capsules.scopes[suit.layout->lineage[n]].constructor;
            if (above == kNoSite || above == receiving || capsules.sites[above].parameters.empty())
                continue;
            return raise_at(satl_line_not_understood,
                            suit.layout->shown + " extends " + capsules.scopes[suit.layout->lineage[n]].layout->shown +
                                ", whose satellite.constructor takes " +
                                std::to_string(capsules.sites[above].parameters.size()) +
                                (capsules.sites[above].parameters.size() == 1 ? " argument" : " arguments") +
                                ", and nothing can hand them over: an object's arguments go to " +
                                suit.layout->shown + "'s own constructor, and every constructor above it runs with none",
                            std::string(), state, row, suit.declared_at, "satl(check)");
        }
    }
    if (held != nullptr)
        return raise_at(held->code, held->why, std::string(), state, registry[held->row], held->at, "satl(check)");
    state.set("program(checked): " + std::to_string(capsules.sites.size()) + " capsules", success);
    return success;
}

} // namespace satellite004
