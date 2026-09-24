// satellite/bytecode/program_walk.cpp -- the header says what the three pieces
// are for and why a call is a position rather than an object. The checker moved
// to program_check.cpp when statements grew past calls, for the author's 300-line
// target -- WHICH THIS FILE NO LONGER MEETS: `while`, `if`/`else` and `for` took
// it to 861 lines. The same move a second time is not a move, because the three
// statement runners and run_statements call EACH OTHER, so lifting them out means
// putting run_statements in a header and promising it to everything that includes
// one. That is the author's to rule (MILESTONES M20.A), not to be done in passing.
//
// A STATEMENT IS ONE OF EIGHT SHAPES, and run_statements below is that list:
//
//     satellite.return(...)                      ends the body
//     satellite.variable.number <name> = <expr>  declares, and gives a value
//                                                (.string .binary .percentage the same)
//     <name> = <expr>                            gives a value to one declared
//     satellite.statement.while(<expr>) { ... }  runs the body while it holds
//     satellite.statement.if(<expr>) { ... }     runs it once, if it holds
//                              [ satellite.statement.else { ... } ]
//     satellite.statement.for(<declaration>; <expr>; <step>) { ... }
//                                                the same loop, counting
//     <word>(<expr>)                             a word of the language
//     <name>()                                   a capsule the user wrote
//
// NOTHING IS ALLOCATED TO RUN A LINE still holds, with one honest exception: a
// body's VariableTable. It is created when the body starts and destroyed when it
// ends, which is what makes "there are no globals" (the author, 2026-09-16) true
// by construction -- a capsule is handed a different table, so it CANNOT see its
// caller's variables even by accident.

#include "program_walk.hpp"

#include "../machine/s_codes.hpp"
#include "../machine/thread_stop.hpp"

#include "statement_ring.hpp"
#include "suit_run.hpp"
#include "main_arguments.hpp"
#include "color_values.hpp"
#include "float_values.hpp"
#include "fraction_values.hpp"
#include "hexadecimal_values.hpp"

#include "word_codes.hpp"
#include "../satl/satl_file.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <mutex>
#include <utility>
#include <sstream>

#include <sys/stat.h>
#include <unistd.h>

namespace satellite004 {
namespace {

using token::Code;

// AN EXPRESSION MUST BE READ TO ITS END (ERROR.md). evaluate_expression stops on
// any code it has no meaning for -- `&` `|` `<<` `!!` are REGISTRY.satellite's
// QUESTION rows -- and hands back the half it read, so `n = 1 & 2` stored 1 and
// `while(n < 3 & 1)` ran as `while(n < 3)`, both without a word. call_word
// already refused this; a variable and a loop bound need it more, because a
// wrong value there prints nothing at all. A trailing comment is the line's end.
bool read_to_the_end(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const Code code = code_at(row, at);
    return code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token;
}

const char *const kNotReadToTheEnd =
    "could not be read to the end -- it stops at something with no meaning there yet "
    "(& | << >> !! are undecided), so the part before it is not the whole value";

// AN EXPRESSION'S REFUSAL, SHOWN ONCE: raise_at, unless a capsule the expression called
// already printed the report (ExpressionContext::reported).
signed long long int raise_context(const ExpressionContext &context, const std::string &doing,
                                   const MachineState &state, const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (context.reported)
        return context.code;
    return raise_at(context.code, context.why, doing, state, row, at);
}

} // namespace

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

std::size_t past_the_statement(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (at < row.size() && code_at(row, at) != token::line_end_token) {
        if (token::carries_a_count(code_at(row, at))) { skip_payload(row, at); continue; }
        ++at;
    }
    return at < row.size() ? at + 1 : at;
}

// The code just past the `}` that closes the `{` at `from`. Counts braces, and
// SKIPS a counted payload rather than reading it -- a string holding a `}` must
// not close a body, which is the same rule that keeps a string saying
// "satellite.main" from declaring one (PROGRESS §6.5).
std::size_t past_matching_brace(const std::vector<std::bitset<16>> &row, std::size_t from)
{
    std::size_t depth = 0;
    std::size_t at = from;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { skip_payload(row, at); continue; }
        if (code == token::left_brace_token) ++depth;
        else if (code == token::right_brace_token && --depth == 0) return at + 1;
        ++at;
    }
    return at;
}

// The `{` that opens a body after `at`, past any line ends. The author writes
// the brace on its own line, so this cannot simply be the next code.
std::size_t brace_after(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (at < row.size() && code_at(row, at) == token::line_end_token) ++at;
    return at;
}

// The header says what this is for. `at` is on the `satellite.statement.for` code.
// Only a semicolon OUTSIDE nested brackets divides the parts, so a call in the
// condition keeps its own commas and brackets; a payload is skipped rather than
// read, by the same rule that keeps a `}` inside a string from closing a body.
ForHeader for_header(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    ForHeader parts;
    std::size_t k = at + 1;
    if (code_at(row, k) != token::left_parenthesis_token)
        return parts;
    parts.declaration = ++k;

    std::size_t depth = 1;
    unsigned int semicolons = 0;
    while (k < row.size()) {
        const Code code = code_at(row, k);
        if (token::carries_a_count(code)) { skip_payload(row, k); continue; }
        if (code == token::line_end_token || code == token::end_of_file_token)
            return ForHeader();                     // the brackets never closed on this line
        if (code == token::left_parenthesis_token) {
            ++depth;
        } else if (code == token::right_parenthesis_token && --depth == 0) {
            parts.closing = k;
            parts.ok = semicolons == 2;
            return parts;
        } else if (code == token::semicolon_token && depth == 1) {
            if (semicolons >= 2)
                return ForHeader();                 // a third `;`: this is not the shape
            (semicolons == 0 ? parts.condition : parts.step) = k + 1;
            ++semicolons;
        }
        ++k;
    }
    return ForHeader();
}

std::size_t return_value_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    std::size_t k = at + 1;
    if (code_at(row, k) != token::left_parenthesis_token)
        return 0;
    ++k;
    const Code first = code_at(row, k);
    if (first == token::right_parenthesis_token)
        return 0;
    if (first == word::code_of(1) && code_at(row, k + 1) == token::right_parenthesis_token)
        return 0;
    return k;
}

bool hands_back_a_value(const BytecodeRegistry &registry, const CapsuleSite &site)
{
    if (site.answers())
        return true;
    if (site.hands_back_known)
        return site.hands_back;
    const std::vector<std::bitset<16>> &row = registry[site.row];
    const std::size_t end = past_matching_brace(row, site.body - 1);
    bool found = false;
    for (std::size_t at = site.body; at < end && !found;) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { skip_payload(row, at); continue; }
        found = code == word::code_of(1, 15) && return_value_at(row, at) != 0;
        ++at;
    }
    site.hands_back = found;
    site.hands_back_known = true;
    return found;
}

// WHAT A for's THIRD PART IS, without running any of it. The step is EXACTLY
// ONE OF THREE THINGS, which is M20.A's own list and not a rule invented here --
// *"here we take as valid input my_int + number, my_int - number, my_int / number,
// my_int * number, my_int ** number(power), my_int % number"*, plus the `++` and
// `--` the same entry asks for:
//
//     (empty)                               the body moves the number itself
//     <name>++   <name>--                   moves_by +1 and -1
//     <name> <+ - * / % ^ **> <expression>  moves_by 0: the evaluator answers it
//
// ANYTHING ELSE IS REFUSED HERE, BY THE CHECKER, BEFORE THE LOOP HAS PRINTED --
// and that is the whole reason this is one rule rather than a list of traps. The
// step is the only part of a for that runs AFTER the body, so a step the walker
// cannot use is a loop that prints a turn and then stops, or worse:
//
//     `--i`        double unary minus, so `i = i`: THE LOOP RAN FOREVER, printing
//                  0 nine million times in five seconds and saying nothing (the
//                  review, 2026-09-17). It is the prefix spelling of `i--`, which
//                  is a spelling the author DID give, so a person will write it.
//     `i`          `i = i`, the same silence.
//     `i * * 2`    named in this file as the wrong thing a generic message sends
//                  a person to write -- and it half-ran until this rule.
//     `i++ + 1`    a doubled sign that is not the whole step.
//     `i & 1`      `&` is a QUESTION row: it has no meaning in an expression yet.
//
// WHAT IS STILL A RUN-TIME REFUSAL: `i + 1 & 2`, where the step BEGINS correctly
// and stops being readable later. That is the same refusal `while(n < 3 & 1)`
// gets (tests/unread_while.satl), and it belongs in the same place as while's.
//
// THE PAYLOAD PROBLEM DISSOLVED WITH THIS RULE. The version before it scanned
// every code of the step looking for `**`, and a payload's codes are never to be
// classified (0x0308 is tight_times_token AND U+0308, a real combining
// character). Nothing is scanned now: only the code straight after the name is
// ever looked at, and a step that starts with a string is refused for not
// starting with the name.
signed long long int for_step_moves_by(const std::vector<std::bitset<16>> &row,
                                       const ForHeader &parts,
                                       const std::string &name,
                                       int &moves_by,
                                       std::string &why)
{
    moves_by = 0;
    if (parts.step == parts.closing)
        return success;                     // the empty step: the third part is the optional one

    const std::string is_written = " -- a for's step is " + name +
                                   " and one of + - * / % ^ ** with a space on both sides, or " + name +
                                   "++ or " + name + "--";

    // ++i and --i, the prefix spelling of the one the author gave. Worth its own
    // sentence because the language accepts the other half of it.
    const Code first = code_at(row, parts.step);
    if ((first == token::tight_plus_token || first == token::tight_minus_token) &&
        first == code_at(row, parts.step + 1)) {
        const std::string doubled = first == token::tight_plus_token ? "++" : "--";
        why = "satellite.statement.for's step is written " + name + doubled + ", not " + doubled + name;
        return satl_line_not_understood;
    }
    if (first != token::name_token) {
        why = "satellite.statement.for's step does not begin with " + name + is_written;
        return satl_line_not_understood;
    }

    std::size_t after = parts.step;
    const std::string moved = text_at(row, after);
    if (moved != name) {
        why = "satellite.statement.for's step moves " + moved + ", which is not " + name +
              ", the number this loop declared";
        return satl_line_not_understood;
    }

    // `<name>++` and `<name>--`, and the doubled sign must be the WHOLE step.
    const Code sign = code_at(row, after);
    if ((sign == token::tight_plus_token || sign == token::tight_minus_token) &&
        sign == code_at(row, after + 1)) {
        const std::string doubled = sign == token::tight_plus_token ? "++" : "--";
        if (after + 2 != parts.closing) {
            why = "satellite.statement.for's " + name + doubled + " is the whole step, and there is more after it" +
                  is_written;
            return satl_line_not_understood;
        }
        moves_by = sign == token::tight_plus_token ? 1 : -1;
        return success;
    }

    // A TOUCHING `**` BY NAME. M20.A lists `my_int ** number(power)`, and since INF-1
    // (SATELLITE_INFINITY.md) a SPACED `**` is power everywhere, this bracket
    // included: the lexer writes it as power_token, so `i ** 2` never reaches this.
    // What does is `i**2`, two touching stars -- and the generic answer ("a math
    // operation needs a space on both sides") would send a person to write
    // `i * * 2`, which is not power either, so this one is named.
    if (sign == token::tight_times_token && code_at(row, after + 1) == token::tight_times_token) {
        why = "in satellite.statement.for, power is written with a space on both sides -- write " + name +
              " ** ... or " + name + " ^ ...";
        return satl_line_not_understood;
    }

    // One of the author's six, SPACED, with something after it for it to work on.
    const bool arithmetic = sign == token::plus_token || sign == token::minus_token ||
                            sign == token::times_token || sign == token::divide_token ||
                            sign == token::modulus_token || sign == token::power_token;
    if (!arithmetic || after + 1 >= parts.closing) {
        why = "satellite.statement.for's step does not move " + name + is_written;
        return satl_line_not_understood;
    }

    // `i * * 2` IS TWO SPACED OPERATORS, not `**` -- both stars have a space on
    // both sides, so the lexer writes two times_tokens and the `**` rule above
    // never sees it. It is the spelling this file names as the wrong thing a
    // generic message sends a person to write, so it does not get to half-run:
    // an operator can never be the value another operator works on. A TOUCHING
    // minus may (`i - -1` is i + 1), which is why only the spaced six are refused.
    const Code next = code_at(row, after + 1);
    if (next == token::plus_token || next == token::minus_token || next == token::times_token ||
        next == token::divide_token || next == token::modulus_token || next == token::power_token) {
        why = "satellite.statement.for's step has two operations in a row and no number between them" + is_written;
        return satl_line_not_understood;
    }
    return success;
}

signed long long int load_program(const std::string &main_file,
                                  StartupThreads &threads,
                                  unsigned long long int batches,
                                  BytecodeRegistry &registry,
                                  BytecodeFilenames &filenames,
                                  MachineState &state)
{
    registry.clear();
    filenames.clear();

    // WHERE RELATIVE NAMES START FROM, fixed now and never again (file_calls.cpp).
    if (program_start_directory().empty()) {
        std::vector<char> here(4096);
        while (getcwd(here.data(), here.size()) == nullptr && errno == ERANGE) here.resize(here.size() * 2);
        if (here.front() == '/') program_start_directory() = here.data();
    }

    // Each file waits with the name of the file that asked for it, so a missing
    // one can say who wanted it.
    std::vector<std::pair<std::string, std::string>> waiting{{main_file, std::string()}};
    std::vector<std::string> loaded;

    while (!waiting.empty()) {
        const std::string path = waiting.front().first;
        const std::string asked_by = waiting.front().second;
        waiting.erase(waiting.begin());

        bool already = false;
        for (const std::string &done : loaded) already = already || done == path;
        if (already)
            continue;   // a cycle of includes ends here instead of running forever
        loaded.push_back(path);

        // CANNOT LOCATE FILE (the author, 2026-09-16), said before load_satl is
        // asked, so the message names the file the program meant rather than
        // whatever the operating system called the failure.
        //
        // ONLY A REGULAR FILE IS A PROGRAM (PLAN M0.5, DESIGN §9). A directory
        // opens and reads as nothing, which surfaced as "missing include" (10)
        // about a file that was never a program; a FIFO blocks the open until
        // something writes to it, and /dev/zero reads until memory runs out. A
        // symlink is followed, so a link to a program runs it. Each refusal says
        // which of those it was.
        const std::string by = asked_by.empty() ? std::string() : ", included by " + asked_by;
        const std::string named = path.empty() ? std::string("(an empty name)") : path;
        struct stat about;
        if (stat(path.c_str(), &about) != 0)
            return report_error((errno == ENOENT || errno == ENOTDIR ? "cannot locate file: " + named
                                                                     : "cannot locate file: " + named + " (" +
                                                                           std::strerror(errno) + ")") + by,
                                missing_satl_file);
        if (!S_ISREG(about.st_mode))
            return report_error("cannot run " + named + by + ": " +
                                    (S_ISDIR(about.st_mode) ? "it is a directory" : "it is not a regular file") +
                                    ", and a program is a .satl file",
                                missing_satl_file);
        std::ifstream there(path);
        if (!there)
            return report_error("cannot read file: " + named + " (" + std::strerror(errno) + ")" + by,
                                missing_satl_file);

        std::string source;
        const signed long long int code = load_satl(path, source, state);
        if (stops_the_program(code))
            return code;

        // THE COPY THAT RUNS (the author, 2026-09-18): the file is read once, here,
        // and a report quotes this copy -- so a program may rewrite its own .satl
        // while it runs and nothing it is doing changes (source_position.hpp).
        loaded_sources()[path] = source;
        add_file_to_bytecode_registry(path, source, threads, batches, registry, filenames);

        // Only the includes are read out of a file at load time. There are no
        // globals, so nothing else in it can run before main does.
        const std::vector<std::bitset<16>> &row = registry.back();
        for (std::size_t i = 0; i < row.size(); ) {
            // A PAYLOAD'S CODES ARE SKIPPED, NEVER CLASSIFIED. A character's own number
            // can be any 16 bits: "ဂ" ends in 0x1002, which is satellite.include's code,
            // and "ဂ"("other") loaded other.satl, whose main then ran instead of this
            // program's (the payload sweep, 2026-09-17).
            if (token::carries_a_count(code_at(row, i))) { skip_payload(row, i); continue; }
            if (code_at(row, i) != word::code_of(1, 1)) { ++i; continue; }
            std::size_t k = i;
            const IncludeShape shape = include_at(row, k, path);
            i = (k > i) ? k : i + 1;
            if (shape.kind == IncludeShape::Kind::none ||
                shape.kind == IncludeShape::Kind::main_marker)
                continue;
            waiting.push_back({shape.resolved, path});
        }
    }

    state.set("program(loaded): " + std::to_string(registry.size()) + " files, " +
                  std::to_string(codes_in(registry)) + " codes", success);
    return success;
}

namespace {

// A CAPSULE'S LAST CALL TO ITSELF IS A LOOP, NOT A DEEPER FRAME (the author,
// 2026-09-22): "Let's allow capsules to call themselves only as the last line ...
// and do the tail call thing, and specifically leave it broken -- we'll just crash
// the interpreter when a capsule calls itself in the middle, adding almost nothing
// to anything this way!" So a capsule whose last act is to call itself hands its
// arguments back to run_site, which lets the frame go and runs the body again --
// the recursion never stops and never grows. A call anywhere else recurses in C++
// as it always has, and past the stack (machine/stack_share.hpp) it crashes: his
// ruling, not an oversight.
//
// LAST MEANS NOTHING OF THIS CAPSULE RUNS AFTER IT: the body's last statement, or
// one followed only by satellite.return, or the same inside an if or else branch
// when that if is itself last. A while or for body is never last -- the loop goes
// round again. Only a call to ITSELF, as he said: a call to another capsule, even
// last, is an ordinary call.
//
// WHICH STATEMENTS ARE LAST IS A SHAPE, so it is worked out once per capsule, the
// first time it runs, into its CapsuleSite -- a call to itself then asks whether it
// stands at one of those places, and nothing else the walker does changes. (It was
// first a walk of every if/else chain each time one ran, and a capsule calling
// itself mid-body paid 5% for it: measured 2026-09-22, and taken out.)
//
// run_site owns one of these for the body it runs and passes it down through every
// block of it. A call inside a while or for body is never one of the last
// statements, so handing the frame into a loop does not make one a tail call.
//
// AND satellite.return ENDS THE CAPSULE, FROM ANY DEPTH, WITH ITS ANSWER (2026-09-22).
// It used to end only the block it stood in: inside a while it went round again, and
// inside an if the capsule carried on after the if (MILESTONES M20's item 1, and the
// help writers' first finding). The author left open whether it leaves the capsule
// and what its argument means; his own programs answer both, written for 003 --
// tagged_report.satl's role_of returns "void" from inside two ifs and "recovery"
// after them, which is only right if a return leaves the capsule with its value.
// So `returned` is set where it stands, every block above it stops, and `answer`
// is what it handed back.
struct Frame {
    const CapsuleSite *site = nullptr;     // the capsule whose body this is; null for a typed line
    std::vector<Value> arguments;          // the next turn's, once it calls itself last
    bool pending = false;
    bool returned = false;                 // satellite.return was reached: the body ends here
    bool answered = false;                 // ...and it handed back a value, in `answer`
    Value answer;
    std::size_t returned_at = 0;           // where that satellite.return stands, for a report

    bool ending() const { return pending || returned; }
};

// Past an if's whole chain -- its body and every else after it, `else if`
// included -- with where each branch's statements begin put in `bodies`. run_if's
// own steps, taken without running anything; the checker has already refused a
// chain missing a brace, and one still ends where run_if would stop.
std::size_t walk_the_if_chain(const std::vector<std::bitset<16>> &row, std::size_t at,
                              std::vector<std::size_t> &bodies)
{
    for (;;) {
        const std::size_t brace = brace_after(row, past_the_statement(row, at));
        if (code_at(row, brace) != token::left_brace_token)
            return past_the_statement(row, at);
        bodies.push_back(brace + 1);
        const std::size_t past = past_matching_brace(row, brace);
        std::size_t next = past;
        while (code_at(row, next) == token::line_end_token) ++next;
        if (code_at(row, next) != word::code_of(1, 13, 4))
            return past;
        const std::size_t after_else = brace_after(row, next + 1);
        if (code_at(row, after_else) == word::code_of(1, 13, 1)) {
            at = after_else;
            continue;
        }
        if (code_at(row, after_else) != token::left_brace_token)
            return next + 1;
        bodies.push_back(after_else + 1);
        return past_matching_brace(row, after_else);
    }
}

void find_the_last_statements(const std::vector<std::bitset<16>> &row, std::size_t from,
                              std::vector<std::size_t> &into, bool ends_it);

constexpr std::size_t kNoStatement = static_cast<std::size_t>(-1);

// `last` IS THE LAST THING THE CAPSULE DOES -- itself, or, when it is an if, the last of
// each of its branches, as deep as they go.
void mark_last(const std::vector<std::bitset<16>> &row, std::size_t last, const std::vector<std::size_t> &bodies,
               std::vector<std::size_t> &into)
{
    if (last == kNoStatement)
        return;
    if (code_at(row, last) != word::code_of(1, 13, 1)) {
        into.push_back(last);
        return;
    }
    for (const std::size_t body : bodies)
        find_the_last_statements(row, body, into, true);
}

// THE STATEMENTS OF THE BLOCK AT `from` THAT ARE THE LAST THING THE CAPSULE DOES: the one
// straight before a satellite.return that hands back nothing -- in ANY block, since a
// return ends the whole capsule wherever it stands (2026-09-22; the review found
// `down(n - 1)` then `satellite.return()` inside an if still recursing) -- and, when
// `ends_it`, the block's own last statement. A loop's body never ends the capsule, the
// loop goes round again, but a call and a return inside it do.
void find_the_last_statements(const std::vector<std::bitset<16>> &row, std::size_t from,
                              std::vector<std::size_t> &into, bool ends_it)
{
    std::size_t last = kNoStatement;
    std::vector<std::size_t> last_bodies;
    for (std::size_t at = from;;) {
        while (code_at(row, at) == token::line_end_token) ++at;
        const Code code = code_at(row, at);
        if (at >= row.size() || code == token::right_brace_token)
            break;
        // A RETURN ENDS THE CAPSULE HERE. One handing back nothing makes what came
        // before it last; ONE THAT HANDS BACK A VALUE IS ITSELF LAST: `f(n - 1)` and then
        // `satellite.return(n)` answers n, and a loop that ran f again instead would
        // answer what the deepest turn did.
        if (code == word::code_of(1, 15)) {
            if (return_value_at(row, at) == 0)
                mark_last(row, last, last_bodies, into);
            return;
        }
        std::vector<std::size_t> bodies;
        const std::size_t here = at;
        if (code == word::code_of(1, 13, 1)) {                                           // if
            at = walk_the_if_chain(row, at, bodies);
            for (const std::size_t body : bodies)
                find_the_last_statements(row, body, into, false);
        } else if (code == word::code_of(1, 13, 3) || code == word::code_of(1, 13, 2)) { // while, for
            const std::size_t brace = brace_after(row, past_the_statement(row, at));
            if (code_at(row, brace) == token::left_brace_token)
                find_the_last_statements(row, brace + 1, into, false);
            at = past_matching_brace(row, brace);
        } else {
            at = past_the_statement(row, at);
        }
        last = here;
        last_bodies = std::move(bodies);
    }
    if (ends_it)
        mark_last(row, last, last_bodies, into);
}

// Does the statement at `at` stand where it is the last thing `site` does.
bool is_last_in(const CapsuleSite &site, const std::vector<std::bitset<16>> &row, std::size_t at)
{
    // FILLED ONCE, AND TWO THREADS MAY REACH IT FIRST TOGETHER (threads, 2026-09-23): the
    // same capsule started on two threads at once calls itself for the first time on both.
    // So the fill is under a lock, and `known` is read and written as an atomic -- through
    // atomic_ref, which leaves CapsuleSite a plain struct. Once known, the vector never
    // changes, so every later read is a plain read.
    std::atomic_ref<bool> known(site.last_statements_known);
    if (!known.load(std::memory_order_acquire)) {
        static std::mutex filling;
        const std::lock_guard<std::mutex> hold(filling);
        if (!known.load(std::memory_order_relaxed)) {
            find_the_last_statements(row, site.body, site.last_statements, true);
            known.store(true, std::memory_order_release);
        }
    }
    for (const std::size_t last : site.last_statements)
        if (last == at)
            return true;
    return false;
}

// Every statement from `from` until the `}` that closes the body, or the row's
// end. `variables` is THIS body's own table. `frame` is the capsule's own (Frame),
// handed through every block of it, so a return anywhere ends the whole body.
signed long long int run_statements(const BytecodeRegistry &registry,
                                    const CapsuleTable &capsules,
                                    const FunctionTable &functions,
                                    std::size_t which_row,
                                    std::size_t from,
                                    VariableTable &variables,
                                    MachineState &state,
                                    Frame &frame);

// `satellite.statement.while(<expr>) { ... }`. `at` is on the word code and is
// left past the body's `}`.
//
// THE CONDITION IS RE-READ FROM THE SAME POSITION EVERY TURN, which is the whole
// loop: the walker keeps a position, so going round again is assigning one
// integer, not rebuilding anything. Nothing is allocated per iteration.
// `satellite.statement.if(<expr>) { ... }`, with every `satellite.statement.else`
// that follows it -- including `else` written straight onto another `if`.
//
// IT IS run_while WITHOUT THE LOOP, which is the whole of the author's point
// (2026-09-17): *"satellite.statement.if is just (condition) { call_to_whatever
// runs_code } which we have kinda just built the thing that runs code"*. The
// condition goes through evaluate_expression and must answer a bool; the body is
// run_statements at the code past the `{`, sharing this body's variables.
//
// `may_run` IS FALSE FOR A BRANCH THE CHAIN HAS ALREADY DECIDED AGAINST, and it
// carries one rule with it: a branch that will not run does not EVALUATE its
// condition either. A condition may call a word, and a call that a person can see
// did not happen must not happen.
signed long long int run_if(const BytecodeRegistry &registry,
                            const CapsuleTable &capsules,
                            const FunctionTable &functions,
                            std::size_t which_row,
                            std::size_t &at,
                            VariableTable &variables,
                            MachineState &state,
                            bool may_run,
                            Frame &frame);

signed long long int run_while(const BytecodeRegistry &registry,
                               const CapsuleTable &capsules,
                               const FunctionTable &functions,
                               std::size_t which_row,
                               std::size_t &at,
                               VariableTable &variables,
                               MachineState &state,
                               Frame &frame)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const std::size_t condition_at = at + 1;          // the `(`
    std::size_t after = past_the_statement(row, at);
    const std::size_t brace = brace_after(row, after);
    if (code_at(row, brace) != token::left_brace_token) {
        at = after;
        return report_error("satl(run): satellite.statement.while has no body", satl_line_not_understood);
    }
    const std::size_t past = past_matching_brace(row, brace);
    at = past;

    for (;;) {
        std::size_t here = condition_at;
        ExpressionContext context{variables, functions, state};
        const bool opened = code_at(row, here) == token::left_parenthesis_token;
        if (opened) ++here;
        const Value holds = evaluate_expression(row, here, context);
        if (context.code != success)
            return raise_context(context, "satellite.statement.while",
                            state, row,
                            context.placed ? context.refused_at : condition_at);
        // The condition's own `)`, then the line's end -- nothing between.
        bool closed = true;
        if (opened) closed = code_at(row, here++) == token::right_parenthesis_token;
        if (!closed || !read_to_the_end(row, here))
            return report_error(std::string("satl(run): satellite.statement.while's condition ") + kNotReadToTheEnd,
                                satl_line_not_understood);
        if (!holds.is_bool())
            return report_error(std::string("satl(run): satellite.statement.while was given ") +
                                    holds.kind_name() + " and needs a true or false",
                                types_do_not_meet);
        if (!*holds.as_bool())
            return success;

        // THE BODY SHARES THIS BODY'S VARIABLES. A while is not a capsule: the
        // author's own program writes `counter = counter + 1` inside one and
        // expects the counter outside it to move.
        const signed long long int code =
            run_statements(registry, capsules, functions, which_row, brace + 1, variables, state, frame);
        if (stops_the_program(code))
            return code;
        if (frame.ending())
            return success;         // a satellite.return in the body left the capsule
    }
}

signed long long int run_if(const BytecodeRegistry &registry,
                            const CapsuleTable &capsules,
                            const FunctionTable &functions,
                            std::size_t which_row,
                            std::size_t &at,
                            VariableTable &variables,
                            MachineState &state,
                            bool may_run,
                            Frame &frame)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const std::size_t condition_at = at + 1;
    const std::size_t after = past_the_statement(row, at);
    const std::size_t brace = brace_after(row, after);
    if (code_at(row, brace) != token::left_brace_token) {
        at = after;
        return report_error("satl(run): satellite.statement.if has no body", satl_line_not_understood);
    }
    const std::size_t past = past_matching_brace(row, brace);
    at = past;

    bool held = false;
    if (may_run) {
        std::size_t here = condition_at;
        ExpressionContext context{variables, functions, state};
        const bool opened = code_at(row, here) == token::left_parenthesis_token;
        if (opened) ++here;
        const Value holds = evaluate_expression(row, here, context);
        if (context.code != success)
            return raise_context(context, "satellite.statement.if",
                            state, row,
                            context.placed ? context.refused_at : condition_at);
        bool closed = true;
        if (opened) closed = code_at(row, here++) == token::right_parenthesis_token;
        if (!closed || !read_to_the_end(row, here))
            return report_error(std::string("satl(run): satellite.statement.if's condition ") + kNotReadToTheEnd,
                                satl_line_not_understood);
        if (!holds.is_bool())
            return report_error(std::string("satl(run): satellite.statement.if was given ") + holds.kind_name() +
                                    " and needs a true or false",
                                types_do_not_meet);
        held = *holds.as_bool();
        if (held) {
            const signed long long int code =
                run_statements(registry, capsules, functions, which_row, brace + 1, variables, state, frame);
            if (stops_the_program(code))
                return code;
        }
    }

    // THE else IS THIS STATEMENT'S, and it is stepped over whether it runs or
    // not: leaving it for run_statements would make it a statement of its own,
    // which is what "an else with no if before it" means.
    std::size_t next = past;
    while (code_at(row, next) == token::line_end_token) ++next;
    if (code_at(row, next) != word::code_of(1, 13, 4))
        return success;

    const std::size_t after_else = brace_after(row, next + 1);
    const bool run_the_else = may_run && !held;
    if (code_at(row, after_else) == word::code_of(1, 13, 1)) {   // else written onto another if
        std::size_t chained = after_else;
        const signed long long int code =
            run_if(registry, capsules, functions, which_row, chained, variables, state, run_the_else, frame);
        at = chained;
        return code;
    }
    if (code_at(row, after_else) != token::left_brace_token) {
        at = next + 1;
        return report_error("satl(run): satellite.statement.else has no body", satl_line_not_understood);
    }
    at = past_matching_brace(row, after_else);
    if (!run_the_else)
        return success;
    return run_statements(registry, capsules, functions, which_row, after_else + 1, variables, state, frame);
}

// THE THIRD PART OF A for, WHICH IS NOT AN EXPRESSION AND NOT AN ASSIGNMENT
// (MILESTONES M20.A). The author: *"we must take any math operation here and then
// add the declared number in the beginning of the statement, so in this example we
// add `my_int = ` to the final block"*. So `my_int + 1` is written without an `=`
// and MEANS `my_int = my_int + 1`: the step is worked out by the ordinary
// evaluator and its answer is given to the loop's own name. `my_int * 2`,
// `my_int - 1`, `my_int ^ 2`, `my_int % 7` all follow, because the evaluator does
// not care which operator it is.
//
// `++` AND `--` ARE THE ONE SPELLING THE LANGUAGE HAS NOWHERE ELSE, which is the
// author's own framing -- *"in a form that is not consistent with other parts of
// the language, so the for loop is the only place where this exists"*. They are
// not tokens and do not become tokens: the lexer already writes `i++` as the name
// and two TOUCHING pluses, and touching is not an operation anywhere in
// satellite, so reading the pair here takes the spelling without giving it a
// meaning outside this bracket. Each is its `+ 1` through the same
// satelliteObject::add that `+` reaches.
//
// WHICH OF THE TWO IT IS, IS A SHAPE, so for_step_moves_by answers it once --
// for the CHECKER before anything runs, and for run_for before its first turn --
// rather than run_for_step working it out again on every turn of the loop.
signed long long int run_for_step(const std::vector<std::bitset<16>> &row,
                                  const ForHeader &parts,
                                  const std::string &name,
                                  int moves_by,
                                  const FunctionTable &functions,
                                  VariableTable &variables,
                                  MachineState &state)
{
    if (parts.step == parts.closing)        // no step: the body moves the number itself
        return success;

    // run_for put the name there and only run_for takes it away, so this cannot
    // fail today. It is asked anyway because the answer is used as a pointer, and
    // a wrong answer here would be a crash rather than a refusal.
    const VariableTable::iterator counting = variables.find(name);
    if (counting == variables.end())
        return report_error("satl(run): satellite.statement.for's " + name + " is no longer declared",
                            name_not_declared);

    Value answer;
    if (moves_by != 0) {
        const Value one = Value::of_number(satellite_number::from_signed(1));
        std::string why;
        const signed long long int code = moves_by > 0 ? counting->second.value.add(one, answer, why)
                                                       : counting->second.value.subtract(one, answer, why);
        if (code != success)
            return report_error("satl(run): in satellite.statement.for, " + why, code);
        counting->second.value = std::move(answer);
        return success;
    }

    std::size_t at = parts.step;
    ExpressionContext context{variables, functions, state};
    answer = evaluate_expression(row, at, context);
    if (context.code != success)
        return raise_context(context, "satellite.statement.for",
                        state, row,
                        context.placed ? context.refused_at : at);
    if (at != parts.closing)
        return report_error(std::string("satl(run): satellite.statement.for's third part ") + kNotReadToTheEnd,
                            satl_line_not_understood);
    if (!answer.is_number())
        return report_error("satl(run): satellite.statement.for's " + name + " was declared " +
                                word::spelling_of(word::code_of(1, 6, 4)) + " and its step answered " +
                                answer.kind_name(),
                            types_do_not_meet);
    counting->second.value = std::move(answer);
    return success;
}

// `satellite.statement.for(satellite.variable.number my_int = 0; my_int < 9; my_int + 1)`
// and then a body -- the author's own line, MILESTONES M20.A. `at` is on the word
// code and is left past the body's `}`.
//
// IT IS A while WITH TWO MORE PARTS, which is the same economy `if` was: the
// condition is read from its own position every turn and goes through the same
// evaluator and the same is_bool() demand, and the body is run_statements sharing
// this body's variables. Nothing is allocated per turn but the step's answer.
//
// THE NUMBER BELONGS TO THE LOOP. The author: *"you must declare a number here
// and then that number exists in 2 places: it exists while the for loop is
// running, then it exists under... satellite.history"*. satellite.history is
// M20.B and is not built, so the first half is what exists: the name is put into
// this body's table before the first turn and TAKEN OUT when the loop ends. It is
// this body's table and not a new one for the same reason a while's body shares
// it -- a loop that could not move the counter outside it would be a capsule.
signed long long int run_for(const BytecodeRegistry &registry,
                             const CapsuleTable &capsules,
                             const FunctionTable &functions,
                             std::size_t which_row,
                             std::size_t &at,
                             VariableTable &variables,
                             MachineState &state,
                             Frame &frame)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const ForHeader parts = for_header(row, at);
    const std::size_t after = past_the_statement(row, at);
    const std::size_t brace = brace_after(row, after);
    if (!parts.ok || code_at(row, brace) != token::left_brace_token) {
        at = after;
        return report_error(parts.ok ? "satl(run): satellite.statement.for has no body"
                                     : "satl(run): satellite.statement.for is written "
                                       "(satellite.variable.number <name> = <value>; <condition>; <step>)",
                            satl_line_not_understood);
    }
    at = past_matching_brace(row, brace);

    // THE FIRST PART DECLARES, and the checker has already said it is a number
    // with a name and an `=`. What cannot be known without running is the VALUE.
    std::size_t k = parts.declaration + 1;
    const std::string name = text_at(row, k);
    ++k;                                                  // past the `=`
    ExpressionContext opening{variables, functions, state};
    Value start = evaluate_expression(row, k, opening);
    if (opening.code != success)
        return raise_context(opening, "satellite.statement.for",
                        state, row,
                        opening.placed ? opening.refused_at : k);
    if (k != parts.condition - 1)
        return report_error(std::string("satl(run): satellite.statement.for's first part ") + kNotReadToTheEnd,
                            satl_line_not_understood);
    if (!start.is_number())
        return report_error("satl(run): satellite.statement.for declares " +
                                std::string(word::spelling_of(word::code_of(1, 6, 4))) + " " + name +
                                ", and it was given " + start.kind_name(),
                            types_do_not_meet);
    variables[name] = Variable{word::code_of(1, 6, 4), plain_shape(word::code_of(1, 6, 4)), std::move(start)};

    // ONCE, NOT PER TURN -- and the checker has already refused the two shapes
    // this can turn down, so a program reaching here has a step that is one of
    // the three (see for_step_moves_by).
    int moves_by = 0;
    std::string shaped;
    const signed long long int step_shape = for_step_moves_by(row, parts, name, moves_by, shaped);
    if (step_shape != success) {
        variables.erase(name);
        return report_error("satl(run): " + shaped, step_shape);
    }

    signed long long int stopped = success;
    for (;;) {
        std::size_t here = parts.condition;
        ExpressionContext turn{variables, functions, state};
        const Value holds = evaluate_expression(row, here, turn);
        if (turn.code != success) {
            stopped = raise_context(turn, "satellite.statement.for",
                               state, row,
                               turn.placed ? turn.refused_at : here);
            break;
        }
        if (here != parts.step - 1) {
            stopped = report_error(std::string("satl(run): satellite.statement.for's condition ") + kNotReadToTheEnd,
                                   satl_line_not_understood);
            break;
        }
        if (!holds.is_bool()) {
            stopped = report_error(std::string("satl(run): satellite.statement.for was given ") + holds.kind_name() +
                                       " and needs a true or false",
                                   types_do_not_meet);
            break;
        }
        if (!*holds.as_bool())
            break;

        stopped = run_statements(registry, capsules, functions, which_row, brace + 1, variables, state, frame);
        if (stops_the_program(stopped) || frame.ending())
            break;
        stopped = run_for_step(row, parts, name, moves_by, functions, variables, state);
        if (stops_the_program(stopped))
            break;
    }

    variables.erase(name);      // the loop is over, and so is its number
    return stops_the_program(stopped) ? stopped : success;
}

// WHAT A CONTAINER IS BEFORE ANYTHING IS PUT IN IT.
//
// `satellite.container.index scores` MUST START AS AN EMPTY INDEX and not as
// nothing, or `scores["alice"] = 10` has nothing to write into and there is no
// other way to fill one -- an index has no literal yet. A list starts empty for
// the same reason, and every other type still starts as nothing, which is what
// name_not_declared already reports when it is read too early.
Value empty_container_for(Code declared)
{
    if (declared == word::code_of(1, 4, 5)) return Value::of_index(make_index());
    if (declared == word::code_of(1, 4, 2)) return Value::of_list(make_list());
    return Value();
}

// `satellite.variable.number <name> = <expr>`, and `<name> = <expr>`.
// `declared` is 0 for a plain assignment.
signed long long int run_assignment(const std::vector<std::bitset<16>> &row,
                                    std::size_t &at,
                                    Code declared,
                                    const TypeShape &shape,
                                    const std::string &name,
                                    const FunctionTable &functions,
                                    VariableTable &variables,
                                    MachineState &state)
{
    // A DECLARATION DOES NOT REFUSE A NAME THAT IS ALREADY THERE, and that is
    // not laxity -- it is the difference between a declaration being reached
    // TWICE IN THE TEXT and being reached twice IN TIME. A declaration inside a
    // while body is one declaration that runs once per turn of the loop:
    //
    //     satellite.statement.while(n < 3)
    //     {
    //         satellite.variable.number m = 0      <- once in the text, 3 times in time
    //
    // Refusing here made that program die on its second iteration, having
    // already printed its first (found by running it, 2026-09-16). The real
    // error -- two declarations of one name in one capsule -- is caught by
    // program_check.cpp BEFORE anything runs, which is the right place for it:
    // the checker walks the text, so it sees each declaration exactly once.
    // A NAME ALREADY DECLARED IS ONE OF THIS BODY'S, OR A FIELD OF ITS OBJECT (value.hpp).
    const Seen found = declared == 0 ? variables.seen(name) : Seen{};
    if (declared == 0 && !found) {
        at = past_the_statement(row, at);
        return report_error("satl(run): " + name + " has no satellite.variable line declaring it", name_not_declared);
    }
    if (code_at(row, at) != token::assign_token) {
        // A declaration with no `=` is a variable with no value yet. Nothing can
        // read it before something writes it, which name_not_declared already says.
        at = past_the_statement(row, at);
        if (declared != 0)
            variables[name] = Variable{declared, shape, empty_container_for(declared)};
        return success;
    }
    ++at;

    ExpressionContext context{variables, functions, state};
    Value value;
    // satellite.variable.color (2026-09-22): A COLOUR NAME READS ITS OWN VALUE -- `c =
    // 000000` and `c = ff00aa` as six hex digits, and `c = x000000, 50` with its
    // transparency after the comma, the author's second way (color_values.cpp). It
    // answers false for every other name, which is read here as it always was.
    if (!color_reads_its_value(declared != 0 ? declared : found.declared, row, at, value, context))
        value = evaluate_expression(row, at, context);
    if (context.code != success) {
        const std::size_t blame = context.placed ? context.refused_at : at;
        at = past_the_statement(row, at);
        return raise_context(context, name + " = ...",
                        state, row, blame);
    }
    if (!read_to_the_end(row, at)) {
        at = past_the_statement(row, at);
        return report_error("satl(run): " + name + " = ... " + kNotReadToTheEnd, satl_line_not_understood);
    }

    // THE DECLARED TYPE OUTLIVES THE LINE THAT WROTE IT. `n = "text"` on a
    // number is refused rather than quietly making n a string (value.hpp).
    const Code holds = declared != 0 ? declared : found.declared;

    // A NUMBER VARIABLE GIVEN A BINARY KEEPS WHAT IT IS WORTH, which is what
    // `satellite.variable.number n = b1010` did when a b literal was a number, so
    // declaring binary a type of its own did not take that program away. The
    // other direction is refused below: a binary is written with its b (the
    // author), and program_check.cpp says so before anything runs.
    if (holds == word::code_of(1, 6, 4) && value.is_binary())
        value = Value::of_number(value.as_binary()->bits);

    // THE FOUR TYPES OF 2026-09-22 SAY WHAT A VALUE BECOMES IN A NAME OF THEIRS --
    // a number given to a float name, a hex given to a colour name -- each in its
    // own bytecode/<name>_values.cpp, and each answers untouched for a name that is
    // not its business.
    for (auto *on_store : {float_on_store, hexadecimal_on_store, color_on_store, fraction_on_store}) {
        std::string refused;
        const signed long long int stored = on_store(holds, value, refused);
        if (stored != success) {
            at = past_the_statement(row, at);
            return report_error(std::string("satl(run): ") + name + " was declared " + word::spelling_of(holds) +
                                    ", and " + refused,
                                stored);
        }
    }

    // ONE TEST FOR EVERY TYPE, THROUGH THE DECLARED SHAPE. This used to be a
    // chain of `word == this && !value.is_that()`, which grew a row per type and
    // could say nothing about what was between a `<` and a `>`. type_shape.hpp
    // answers both, and answers them the same way for the checker.
    const TypeShape &against = declared != 0 ? shape : *found.shape;
    std::string why;
    if (against.word != 0 && !value_fits(against, value, why)) {
        at = past_the_statement(row, at);
        return report_error(std::string("satl(run): ") + name + " was declared " + shape_written(against) + ", and " +
                                why,
                            types_do_not_meet);
    }
    // A NAME ALREADY THERE KEEPS ITS DECLARATION AND TAKES THE VALUE -- in this body's
    // table, or in the field of the object, which every holder of it then sees.
    if (declared == 0)
        *found.value = std::move(value);
    else
        variables[name] = Variable{holds, against, std::move(value)};
    at = past_the_statement(row, at);
    return success;
}

// `<name>[i] = <expr>`, and `<name>[i][j] = <expr>` (the author, 2026-09-18:
// "we must build it to be able to access lists inside of lists").
//
// `at` IS ON THE FIRST `[`. Every index is worked out first and the value last,
// which is the order they are written in and the order a person would expect a
// refusal in: `a[nope] = 1` complains about `nope` before anything else.
//
// THE WALK ITSELF IS write_through_index, IN expression.cpp, so that reading
// `a[i]` and writing `a[i]` cannot come to disagree about what `i` means.
// IS THERE AN `=` AFTER THE BRACKETS? `at` is on the first `[`; this looks past
// every balanced `[...]` group and answers what it finds, without moving `at`.
//
// IT COUNTS DEPTH RATHER THAN FINDING THE NEXT `]`, because an index is a whole
// expression and may hold brackets of its own: `a[b[1]] = x` is two groups, not
// one, and a scan for the first `]` would stop inside the inner one and see `]`
// where it wanted `=`.
bool assign_after_the_brackets(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (code_at(row, at) == token::left_square_bracket_token) {
        std::size_t depth = 0;
        while (at < row.size()) {
            const Code code = code_at(row, at);
            if (code == token::line_end_token || code == token::end_of_file_token) return false;
            if (token::carries_a_count(code)) { skip_payload(row, at); continue; }
            if (code == token::left_square_bracket_token) ++depth;
            else if (code == token::right_square_bracket_token && --depth == 0) { ++at; break; }
            ++at;
        }
        if (depth != 0) return false;        // never closed on this line
    }
    return code_at(row, at) == token::assign_token;
}

signed long long int run_indexed_assignment(const std::vector<std::bitset<16>> &row,
                                            std::size_t &at,
                                            const std::string &name,
                                            const FunctionTable &functions,
                                            VariableTable &variables,
                                            MachineState &state)
{
    const std::size_t opened_at = at;
    const Seen found = variables.seen(name);     // this body's, or a field of its object
    if (!found) {
        at = past_the_statement(row, at);
        return report_error("satl(run): " + name + " has no satellite.variable line declaring it", name_not_declared);
    }

    ExpressionContext context{variables, functions, state};
    std::vector<Value> indices;
    while (code_at(row, at) == token::left_square_bracket_token) {
        ++at;
        indices.push_back(evaluate_expression(row, at, context));
        if (context.code != success)
            break;
        if (code_at(row, at) != token::right_square_bracket_token) {
            context.refuse(satl_line_not_understood,
                           name + "[...] was given something it could not read to the end of", opened_at);
            break;
        }
        ++at;
    }

    // A GUARD, AND IT IS NAMED AS ONE because it cannot fire today: the caller
    // only reaches this function when assign_after_the_brackets() has already
    // found the `=`. It is here so that a future caller which has not made that
    // check refuses rather than reading a value from wherever `at` happens to
    // stop -- and it is NOT presented as the message a person will meet, because
    // claiming a refusal that never runs is how dead code gets believed.
    if (context.code == success && code_at(row, at) != token::assign_token)
        context.refuse(satl_line_not_understood,
                       name + "[...] needs an = and a value after it", opened_at);

    // `args["username"] = x` IS `args.username = x` (main_arguments.hpp), refused as that is
    // -- after the key filing was fixed it would have overwritten satl's row -- and, with one
    // key, written as that is, so `args["access"] = 7` meets the setting's own rule.
    satellite_string key_text;
    std::string unused, row_key;
    if (context.code == success && found.declared == word::code_of(1, 6, 21) && !indices.empty() &&
        indices.front().kind() == satelliteObject::string && indices.front().to_string(key_text, unused) == success) {
        row_key = key_text.to_utf8();
        const std::string refused =
            why_an_argument_is_not_written(row_key, name, found.value, state.arguments, functions);
        if (!refused.empty())
            context.refuse(word_takes_no_assignment, refused, opened_at);
    }

    if (context.code == success) {
        ++at;
        Value value = evaluate_expression(row, at, context);
        if (context.code == success && !read_to_the_end(row, at))
            context.refuse(satl_line_not_understood, name + "[...] = ... " + kNotReadToTheEnd, opened_at);
        if (context.code == success && !row_key.empty() && indices.size() == 1)
            write_an_argument(row_key, std::move(value), name, *found.value, context);
        else if (context.code == success)
            write_through_index(*found.value, indices, std::move(value), name, opened_at, *found.shape, context);
    }

    const std::size_t blame = context.placed ? context.refused_at : opened_at;
    at = past_the_statement(row, at);
    if (context.code != success)
        return raise_context(context, name + "[...] = ...", state, row, blame);
    return success;
}

// `argz.some_var = <expr>` -- A ROW OF THE ARGUMENTS VARIABLE BEING WRITTEN (the author,
// 2026-09-23), and `argz.l[1] = <expr>`, an item of a row that holds a container. `at` is
// on the `.` after `name`; the run of row names ends on `=` or `[`. main_arguments.hpp
// says which rows a program may write and what writing one does; this is the statement
// around it, in run_setting_assignment's order: the row is judged before the value is
// worked out, so a refused row never asks anybody for input.
signed long long int run_argument_assignment(const std::vector<std::bitset<16>> &row,
                                             std::size_t &at,
                                             const std::string &name,
                                             Value &arguments,
                                             const FunctionTable &functions,
                                             VariableTable &variables,
                                             MachineState &state)
{
    const std::size_t opened_at = at;
    std::string key;
    at = past_the_argument_names(row, at, key);      // on the `=`, or the first `[`
    ExpressionContext context{variables, functions, state};
    std::vector<Value> indices;
    while (context.code == success && code_at(row, at) == token::left_square_bracket_token) {
        ++at;
        indices.push_back(evaluate_expression(row, at, context));
        if (context.code == success && code_at(row, at) != token::right_square_bracket_token)
            context.refuse(satl_line_not_understood,
                           name + "." + key + "[...] was given something it could not read to the end of", opened_at);
        ++at;
    }
    std::size_t blame = opened_at;
    const std::string refused =
        indices.empty() ? why_an_argument_is_not_written(key, name, &arguments, state.arguments, functions) : "";
    if (!refused.empty())
        context.refuse(word_takes_no_assignment, refused, opened_at);
    Value *slot = nullptr;
    if (context.code == success && !indices.empty())
        slot = an_argument_to_change(key, name, arguments, context);
    if (context.code == success) {
        blame = ++at;                                // `a.access = 2` is refused under the 2
        Value value = evaluate_expression(row, at, context);
        if (context.code == success && !read_to_the_end(row, at))
            context.refuse(satl_line_not_understood, name + "." + key + " = ... " + kNotReadToTheEnd, opened_at);
        if (context.code == success && slot != nullptr)
            write_through_index(*slot, indices, std::move(value), name + "." + key, opened_at, TypeShape{}, context);
        else if (context.code == success)
            write_an_argument(key, std::move(value), name, arguments, context);
    }
    at = past_the_statement(row, at);
    if (context.code != success)
        return raise_context(context, name + "." + key + (indices.empty() ? "" : "[...]") + " = ...", state, row,
                             context.placed ? context.refused_at : blame);
    return success;
}

// `<word> = <expr>` -- a setting being written. `arguments.access = true`.
//
// THE THIRD THING A STATEMENT CAN START WITH. Until this, a word at the start of
// a line was a declaration (`<word> <name>`) or a call (`<word>(`), and those
// two were told apart by the token after the word. A setting is the third, told
// apart the same way: the token after it is `=`.
//
// IT IS NOT run_assignment(). That one writes a VariableTable entry and its
// whole job is the declared type outliving the line -- there is no declaration
// here, no name, and nothing in the table. The two share the shape `x = expr`
// and nothing else.
signed long long int run_setting_assignment(const std::vector<std::bitset<16>> &row,
                                            std::size_t &at,
                                            Code code,
                                            const FunctionTable &functions,
                                            VariableTable &variables,
                                            MachineState &state)
{
    const std::string spelling = word::spelling_of(code);
    const NumberRow *library = functions[code];

    // REFUSED BEFORE THE RIGHT-HAND SIDE IS EVALUATED, and that order is the
    // point: `arguments.machine.cores = satellite.console.input("n")` must not
    // ask a person for a number and THEN say the word cannot be written.
    if (library == nullptr || library->scenarios.flag_setting == nullptr)
        return report_error("satl(run): " + spelling + " is not a setting a program can write to",
                            word_takes_no_assignment);

    ++at;                                   // past the word
    ++at;                                   // past the `=`

    ExpressionContext context{variables, functions, state};
    Value value = evaluate_expression(row, at, context);
    if (context.code != success)
        return raise_context(context, spelling + " = ...",
                        state, row,
                        context.placed ? context.refused_at : at);
    if (!read_to_the_end(row, at))
        return report_error("satl(run): " + spelling + " = ... " + kNotReadToTheEnd, satl_line_not_understood);

    // A TRUE/FALSE SETTING TAKES TRUE OR FALSE AND NOTHING ELSE. 1 and 0 are not
    // quietly taken for them: the author's own spelling is
    // `satellite.variable.bool history_valve = true/false`, and a number that
    // silently meant true would make `arguments.access = 2` a line with no
    // meaning that ran anyway.
    if (!value.is_bool())
        return report_error("satl(run): " + spelling + " is true or false, and was given " + value.kind_name(),
                            setting_is_not_a_flag);

    const SettingReply said = library->scenarios.flag_setting(true, *value.as_bool());
    if (said.code != success)
        return report_error("satl(run): " + spelling + " could not be written" +
                                (said.reason.empty() ? "" : " -- " + said.reason),
                            said.code);
    return success;
}

// THE FILES A BODY HELD ARE SAVED AND CLOSED WHEN THE BODY ENDS, AND A SAVE THAT
// FAILS IS SAID (the review, 2026-09-18). A handle's own end saves it too, but a
// destructor has no one to tell: a program whose file had become read-only printed
// `true` for its append, exited 0, and the line was never on the disk. That broke
// SATELLITE_FILE_OPERATIONS 3.1's promise that a user who forgets close loses
// nothing -- they lost it, silently.
//
// A FILE HELD BY ANOTHER NAME IS LEFT TO THAT NAME'S END: the count of names for it
// in this table is compared with the handle's own count, and only a file every one
// of whose holders is ending here is closed here. `stopped` is the body's own
// answer: a failed save becomes the answer only when the body had none.
signed long long int close_files(VariableTable &variables, signed long long int stopped)
{
    std::unordered_map<const satellite_file *, long> held_here;
    for (const std::pair<const std::string, Variable> &entry : variables)
        if (const FileHandle *handle = std::get_if<FileHandle>(&entry.second.value.held))
            if (*handle != nullptr) ++held_here[handle->get()];
    signed long long int answer = stopped;
    for (const std::pair<const std::string, Variable> &entry : variables) {
        const FileHandle *handle = std::get_if<FileHandle>(&entry.second.value.held);
        if (handle == nullptr || *handle == nullptr) continue;
        const std::unordered_map<const satellite_file *, long>::iterator counted = held_here.find(handle->get());
        if (counted == held_here.end() || handle->use_count() != counted->second) continue;
        held_here.erase(counted);
        satellite_file &file = **handle;
        if (!file.ok() || file.close()) continue;
        const signed long long int code =
            report_error("satl(end): the changes to " + file.path() + " could not be saved when " + entry.first +
                             " went out of use -- " + file.error(),
                         file_unwritable);
        if (!stops_the_program(answer)) answer = code;
    }
    return answer;
}

// WHAT A DOTTED NAME AT `at` REACHES, from the scope the statement stands in -- or
// nothing, when it is not a name, the names are not followed by `(`, or the first
// name is not a file or a space. The same CapsuleTable::reach the checker asked.
Reached reached_from(const CapsuleTable &capsules, std::size_t which_row, std::size_t at,
                     const std::vector<std::bitset<16>> &row)
{
    std::vector<std::string> names;
    std::size_t k = at;
    if (!dotted_names_at(row, k, names) || names.size() < 2 || code_at(row, k) != token::left_parenthesis_token)
        return Reached();
    return capsules.reach(capsules.scope_at(which_row, at), names);
}

// ONE CAPSULE, BY ITS SITE: a new frame, the arguments bound, close_files on the way
// out. Every road to a capsule ends here -- a call written in a program, a capsule
// inside an expression, a method on an object, a button's press through run_capsule
// -- so there is one reader of "call a capsule". `self` is the object a spacesuit's
// capsule runs on, null for any other; `answer` is where what it hands back goes,
// null when nothing will read it.
signed long long int run_site(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              const CapsuleSite &site,
                              std::vector<Value> arguments,
                              const UserDefinedHandle &self,
                              MachineState &state,
                              Value *answer,
                              bool *answered = nullptr);

// A CAPSULE CALL STANDING AS A STATEMENT -- `greet(1, 2)` or `other.tools.greet(1, 2)`,
// `at` on its first name and left past the statement. Its arguments are worked out
// HERE, IN THE CALLER'S FRAME (2026-09-21), which is the only frame they could be
// worked out in: the capsule's own has nothing in it yet, and that is the point.
//
// A SPACESUIT'S CAPSULE CALLED BY ITS BARE NAME runs on THIS body's object -- the
// author's `call_open()` inside another of run_log's capsules. The checker has proved
// the caller is a capsule of the same spacesuit, so `variables.self` is that object.
signed long long int call_capsule(const BytecodeRegistry &registry,
                                  const CapsuleTable &capsules,
                                  const FunctionTable &functions,
                                  const CapsuleSite &site,
                                  const std::vector<std::bitset<16>> &row,
                                  std::size_t &at,
                                  VariableTable &variables,
                                  MachineState &state,
                                  Frame &frame)
{
    const std::size_t started = at;
    std::vector<std::string> names;
    std::size_t k = at;
    dotted_names_at(row, k, names);                // k is on the `(`: both callers saw it there
    std::string written = names.front();
    for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];

    std::vector<Value> arguments;
    ExpressionContext context{variables, functions, state};
    std::size_t a = k + 1;
    if (code_at(row, a) != token::right_parenthesis_token) {
        for (;;) {
            arguments.push_back(evaluate_expression(row, a, context));
            if (context.code != success || code_at(row, a) != token::comma_token)
                break;
            ++a;
        }
    }
    if (context.code != success) {
        const std::size_t blame = context.placed ? context.refused_at : at;
        at = past_the_statement(row, k);
        return raise_context(context, written + "(...)", state, row, blame);
    }
    if (code_at(row, a) != token::right_parenthesis_token) {
        at = past_the_statement(row, k);
        return report_error("satl(run): " + written + "(...) " + kNotReadToTheEnd, satl_line_not_understood);
    }
    at = past_the_statement(row, k);
    // ITS OWN CAPSULE, CALLED LAST: the arguments were worked out in this frame,
    // which is all the frame was still needed for, so run_site takes them and
    // lets it go (Frame).
    const UserDefinedHandle none;
    const UserDefinedHandle &self = site.suit != kNoScope ? variables.self : none;
    const CapsuleSite &runs = capsules.on_the_object(site, self);
    if (frame.site == &runs && is_last_in(runs, row, started)) {
        frame.arguments = std::move(arguments);
        frame.pending = true;
        return success;
    }
    return run_site(registry, capsules, functions, runs, std::move(arguments), self, state, nullptr);
}

} // namespace

signed long long int run_declaration(const BytecodeRegistry &registry,
                                     const CapsuleTable &capsules,
                                     const FunctionTable &functions,
                                     std::size_t which_row,
                                     std::size_t &at,
                                     VariableTable &variables,
                                     MachineState &state,
                                     bool as_a_field,
                                     bool &was_one)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const Code code = code_at(row, at);
    was_one = true;

    // A DECLARATION WITH TYPES BETWEEN < AND > (the author, 2026-09-18):
    // `satellite.container.index<satellite.variable.string, satellite.variable.number> scores`.
    // Read as a shape first, because the name is on the far side of the `>`
    // and the plain test below looks only at the next code.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::less_than_token) {
        std::size_t k = at;
        TypeShape shape;
        unsigned int pending = 0;
        std::string why;
        if (!read_type_shape(row, k, shape, pending, why) || pending != 0) {
            if (pending != 0)
                why = "there is a > here with nothing left for it to close";
            at = past_the_statement(row, at);
            report_error("satl(run): " + why, satl_line_not_understood);
            return success;
        }
        if (code_at(row, k) != token::name_token) {
            at = past_the_statement(row, at);
            report_error(std::string("satl(run): ") + word::spelling_of(code) +
                             "<...> declares a name, and there is no name after the >",
                         satl_line_not_understood);
            return success;
        }
        // `satellite.container.list<data_unit> units` NAMES A SPACESUIT, and which one is
        // the scope table's to say, from where the line stands (2026-09-22).
        if (names_a_suit(shape) && !resolve_shape(capsules, capsules.scope_at(which_row, at), shape, why)) {
            const std::size_t started = at;
            at = past_the_statement(row, at);
            return raise_at(name_not_declared, why, std::string(), state, row, started);
        }
        const std::string name = text_at(row, k);
        at = k;
        const signed long long int stopped =
            run_assignment(row, at, code, shape, name, functions, variables, state);
        return stopped;
    }

    // A DECLARATION IS A WORD FOLLOWED BY A NAME; a call is a word followed
    // by `(`. That one test tells them apart with no list of types.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::name_token) {
        std::size_t k = at + 1;
        const std::string name = text_at(row, k);
        at = k;
        const signed long long int stopped =
            run_assignment(row, at, code, plain_shape(code), name, functions, variables, state);
        return stopped;
    }

    // A SPACESUIT'S NAME AND THEN A NAME: an object (suit_run.cpp).
    if (an_object_declaration_at(row, at))
        return run_object_declaration(registry, capsules, functions, which_row, at, variables, state, as_a_field);

    was_one = false;
    return success;
}

namespace {

signed long long int run_statements(const BytecodeRegistry &registry,
                                    const CapsuleTable &capsules,
                                    const FunctionTable &functions,
                                    std::size_t which_row,
                                    std::size_t from,
                                    VariableTable &variables,
                                    MachineState &state,
                                    Frame &frame)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];

    for (std::size_t at = from; at < row.size(); ) {
        const Code code = code_at(row, at);

        // A THREAD ASKED TO STOP stops HERE, between two statements and never inside one
        // (machine/thread_stop.hpp). The main thread's pointer is null. BEFORE the `}` test,
        // so a loop whose body is empty -- `while(satellite.bool.true) { }`, or one whose
        // work is all in its condition -- passes it on every turn: after the test, such a
        // loop could never be stopped, and join() and the end of the run waited forever
        // (the review, 2026-09-23).
        if (stop_of_this_thread != nullptr && stop_of_this_thread->load(std::memory_order_relaxed))
            return thread_stopped;

        if (code == token::right_brace_token)
            return success;
        if (code == token::line_end_token) { ++at; continue; }

        // THE `statements` BIT. Recorded here and nowhere else: this is the top
        // of the one loop every statement passes through, so one site records
        // everything and there is no second place to keep in step.
        //
        // AFTER the two steps above, so a `}` and a bare line end are not counted
        // as statements -- a person reading the ring wants the lines they wrote.
        //
        // A POSITION, NOT A LINE. Counting line_end_tokens is O(n) and this is
        // per statement; statement_ring.hpp says why the counting waits for the
        // report. NOT HOISTED YET -- M35 and F5b are where RunPlan::plain gets a
        // loop with this line compiled out.
        if (state.features.on(Feature::statements)) {
            statement_ring().ready();
            statement_ring().saw(which_row, at);
        }

        // satellite.return, AND WHAT IT HANDS BACK (Frame says why it ends the whole
        // capsule). Worked out in THIS frame, before it goes; run_site measures it
        // against the capsule's satellite.returns.
        if (code == word::code_of(1, 15)) {
            const std::size_t value_at = return_value_at(row, at);
            if (value_at != 0) {
                ExpressionContext context{variables, functions, state};
                std::size_t k = value_at;
                Value handed = evaluate_expression(row, k, context);
                if (context.code != success)
                    return raise_context(context, "satellite.return(...)", state, row,
                                    context.placed ? context.refused_at : value_at);
                if (code_at(row, k) != token::right_parenthesis_token || !read_to_the_end(row, k + 1))
                    return report_error(std::string("satl(run): satellite.return(...) ") + kNotReadToTheEnd,
                                        satl_line_not_understood);
                frame.answer = std::move(handed);
                frame.answered = true;
            }
            frame.returned = true;
            frame.returned_at = at;
            state.set("satellite.return", success);
            return success;
        }

        if (code == word::code_of(1, 13, 1)) {       // satellite.statement.if
            const signed long long int stopped =
                run_if(registry, capsules, functions, which_row, at, variables, state, true, frame);
            if (stops_the_program(stopped))
                return stopped;
            if (frame.ending())
                return success;
            continue;
        }

        // AN else REACHED AS A STATEMENT IS ONE NO if CLAIMED: a real one is
        // stepped over by the if above it. The checker refuses it first, so
        // nothing has run by the time anyone sees this.
        if (code == word::code_of(1, 13, 4)) {
            at = past_the_statement(row, at);
            return report_error("satl(run): satellite.statement.else with no satellite.statement.if before it",
                                satl_line_not_understood);
        }

        if (code == word::code_of(1, 13, 3)) {       // satellite.statement.while
            const signed long long int stopped =
                run_while(registry, capsules, functions, which_row, at, variables, state, frame);
            if (stops_the_program(stopped))
                return stopped;
            if (frame.ending())
                return success;
            continue;
        }

        if (code == word::code_of(1, 13, 2)) {       // satellite.statement.for
            const signed long long int stopped =
                run_for(registry, capsules, functions, which_row, at, variables, state, frame);
            if (stops_the_program(stopped))
                return stopped;
            if (frame.ending())
                return success;
            continue;
        }

        // A DECLARATION: a word and a name, a word and its <>, or a spacesuit's name and a
        // name (run_declaration, below, is the one reader of all three).
        if (word::is_word_code(code) &&
            (code_at(row, at + 1) == token::less_than_token || code_at(row, at + 1) == token::name_token)) {
            bool was_one = false;
            const signed long long int stopped =
                run_declaration(registry, capsules, functions, which_row, at, variables, state, false, was_one);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        // A WORD FOLLOWED BY `=` IS A SETTING BEING WRITTEN, and this test comes
        // before the call below for the reason the declaration test comes before
        // it too: `arguments.access = true` is a word, so the call arm would
        // take it, call it with no argument, and skip past the `= true` without
        // a word said.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::assign_token) {
            std::size_t k = at;
            const signed long long int stopped =
                run_setting_assignment(row, k, code, functions, variables, state);
            at = past_the_statement(row, k);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        // A word of the language: its code IS the function table's index, and
        // its arguments are evaluated inner to outer.
        //
        // THROUGH THE WHOLE EXPRESSION READER, AND READ TO THE END (the review,
        // 2026-09-18): this arm called call_word and then skipped to the next line,
        // so `satellite.file.open("t.se").append("x")` opened the file and dropped
        // the append without a word, and anything after a word's `)` was never
        // looked at. A refusal is placed on the statement's own start when it did
        // not say where -- `at` had already moved to the NEXT line.
        if (word::is_word_code(code)) {
            ExpressionContext context{variables, functions, state};
            const std::size_t started = at;
            std::size_t k = at;
            evaluate_expression(row, k, context);
            at = past_the_statement(row, k);
            if (context.code != success)
                return raise_context(context, std::string(),
                                state, row,
                                context.placed ? context.refused_at : started);
            if (!read_to_the_end(row, k))
                return raise_at(satl_line_not_understood,
                                std::string(word::spelling_of(code)) + "(...) " + kNotReadToTheEnd, std::string(),
                                state, row, k);
            continue;
        }

        if (code == token::name_token) {
            // AN OBJECT BEING DECLARED -- `run_log log(path)`, `tagged_report.run_signal
            // signal` -- a name and then a name, which no other statement starts with.
            if (an_object_declaration_at(row, at)) {
                const signed long long int stopped =
                    run_object_declaration(registry, capsules, functions, which_row, at, variables, state, false);
                if (stops_the_program(stopped))
                    return stopped;
                continue;
            }

            std::size_t k = at;
            const std::string name = text_at(row, k);

            // A METHOD CALL STANDING ALONE (FO-1): worked out as an expression and
            // its answer let go. The expression must reach the line's end, for the
            // same reason an assignment's must.
            // `a[i] = v` IS AN ASSIGNMENT, NOT AN EXPRESSION STANDING ALONE, and
            // the two start identically -- so which it is can only be told by
            // looking past the brackets for an `=`. Without this, `a[1] = "x"`
            // was read as the expression `a[1]`, whose value was thrown away,
            // and the `= "x"` then failed as "not read to the end": a refusal
            // that names the wrong half of the line.
            if (code_at(row, k) == token::left_square_bracket_token &&
                assign_after_the_brackets(row, k)) {
                std::size_t b = k;
                const signed long long int stopped =
                    run_indexed_assignment(row, b, name, functions, variables, state);
                at = b;
                if (stops_the_program(stopped))
                    return stopped;
                continue;
            }

            // A CAPSULE REACHED THROUGH A FILE OR A SPACE (2026-09-22): `other.greet()`,
            // `tools.x()`. It starts exactly as a method call does, so it is told apart
            // by what the first name IS -- a variable is a method's receiver, and a
            // name no variable has that reaches a capsule is the capsule. The checker
            // refuses a variable named like a file or a space, so the two cannot meet.
            const Reached dotted = code_at(row, k) == token::method_token && !variables.seen(name)
                                       ? reached_from(capsules, which_row, at, row)
                                       : Reached();
            if (dotted.site != nullptr) {
                const signed long long int stopped =
                    call_capsule(registry, capsules, functions, *dotted.site, row, at, variables, state, frame);
                if (stops_the_program(stopped))
                    return stopped;
                if (frame.ending())
                    return success;
                continue;
            }

            // `argz.some_var = value` (run_argument_assignment). It starts as a method call
            // does, and is told apart by the `=` after the run of row names -- looked for
            // first, so a method call pays for the look and not for a second lookup.
            const std::size_t past_rows = code_at(row, k) == token::method_token ? past_the_argument_names(row, k) : k;
            if (past_rows != k && (code_at(row, past_rows) == token::assign_token ||
                                   (code_at(row, past_rows) == token::left_square_bracket_token &&
                                    assign_after_the_brackets(row, past_rows)))) {
                const Seen arguments = variables.seen(name);
                if (arguments && arguments.declared == word::code_of(1, 6, 21)) {
                    std::size_t b = k;
                    const signed long long int stopped =
                        run_argument_assignment(row, b, name, *arguments.value, functions, variables, state);
                    at = b;
                    if (stops_the_program(stopped))
                        return stopped;
                    continue;
                }
            }

            if (code_at(row, k) == token::method_token || code_at(row, k) == token::left_square_bracket_token) {
                ExpressionContext context{variables, functions, state};
                context.statement = true;      // `log.call_append(x)`: its answer is let go
                std::size_t e = at;
                evaluate_expression(row, e, context);
                if (context.code != success) {
                    const std::size_t blame = context.placed ? context.refused_at : at;
                    at = past_the_statement(row, e);
                    return raise_context(context, name, state, row, blame);
                }
                if (!read_to_the_end(row, e)) {
                    at = past_the_statement(row, e);
                    return report_error("satl(run): " + name + "... " + kNotReadToTheEnd, satl_line_not_understood);
                }
                at = past_the_statement(row, e);
                continue;
            }

            // A name followed by `(` is a capsule; a name followed by `=` is an
            // assignment. Nothing else is a statement a name can start.
            if (code_at(row, k) == token::left_parenthesis_token) {
                const CapsuleSite *found = capsules.bare(capsules.scope_at(which_row, at), name);
                if (found == nullptr) {
                    at = past_the_statement(row, k);
                    report_error("satl(run): no capsule named " + name, satl_line_not_understood);
                    continue;
                }
                const signed long long int stopped =
                    call_capsule(registry, capsules, functions, *found, row, at, variables, state, frame);
                if (stops_the_program(stopped))
                    return stopped;
                if (frame.ending())
                    return success;
                continue;
            }
            at = k;
            const signed long long int stopped =
                run_assignment(row, at, 0, TypeShape{}, name, functions, variables, state);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        if (token::carries_a_count(code)) { std::size_t k = at; skip_payload(row, k); at = k; continue; }
        ++at;
    }
    return success;
}

// ONE TURN OF A CAPSULE: its frame, the arguments bound into it, the body, and
// close_files on the way out.
signed long long int run_turn(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              const CapsuleSite &site,
                              std::vector<Value> &arguments,
                              const UserDefinedHandle &self,
                              MachineState &state,
                              Frame &frame)
{
    VariableTable theirs;   // its own frame, as every capsule called by name has
    theirs.self = self;     // and a spacesuit's capsule, its object (value.hpp)
    if (site.suit != kNoScope)
        theirs.fields_seen = capsules.scopes[site.suit].layout->fields.size();

    // THE ARGUMENTS BECOME THE FIRST NAMES IN THAT FRAME, and they are the only
    // names it starts with: there are no globals, so what was handed in is the
    // whole of what a capsule can see of the world outside it.
    //
    // MEASURED AGAINST THE DECLARED TYPE, through the same value_fits() a
    // satellite.variable line uses -- so `when_pressed(satellite.variable.number n)`
    // handed a window is refused in the same words, and not quietly bound.
    const std::vector<CapsuleParameter> &wants = site.parameters;
    if (arguments.size() != wants.size())
        return report_error("satl(run): " + site.shown + " takes " + std::to_string(wants.size()) +
                                (wants.size() == 1 ? " argument, and was given " : " arguments, and was given ") +
                                std::to_string(arguments.size()),
                            satl_line_not_understood);
    for (std::size_t at = 0; at < arguments.size(); ++at) {
        std::string why;
        if (!value_fits(wants[at].shape, arguments[at], why))
            return report_error("satl(run): " + site.shown + "'s " + wants[at].name + " was declared " +
                                    shape_written(wants[at].shape) + ", and " + why,
                                types_do_not_meet);
        theirs[wants[at].name] = Variable{wants[at].declared(), wants[at].shape, std::move(arguments[at])};
    }
    return close_files(theirs,
                       run_statements(registry, capsules, functions, site.row, site.body, theirs, state, frame));
}

// ONE TURN PER CALL, and a call to itself made last is the next turn rather than a
// frame inside this one (Frame): the frame, its files and its names go at the
// end of every turn, exactly as they would have at the end of a call.
//
// WHAT IT HANDS BACK IS MEASURED AGAINST ITS satellite.returns ON EVERY TURN, read or
// not -- a capsule answering the wrong type is wrong whether or not this caller
// looked. And a turn that CALLED ITSELF LAST answers nothing, however the deepest
// turn ended: written as the recursion it replaces, the outer call runs `f(n - 1)`,
// drops its answer and reaches its `}`. `satellite.return(f(n - 1))` is a different
// statement, and is a call in the middle (the author's ruling, 2026-09-22).
//
// `answered`, WHEN IT IS GIVEN, MAKES THE ANSWER OPTIONAL (threads, 2026-09-23): a thread's
// capsule that ends without a value is not a fault -- its join() answers nothing -- so
// instead of refusing with capsule_gave_no_answer this says whether there was one.
signed long long int run_site(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              const CapsuleSite &site,
                              std::vector<Value> arguments,
                              const UserDefinedHandle &given,
                              MachineState &state,
                              Value *answer,
                              bool *answered)
{
    // THE OBJECT IS HELD BY A HANDLE OF ITS OWN FOR EVERY TURN: `given` is a reference
    // into whoever called, and a capsule may give the name it came from another object.
    const UserDefinedHandle self = given;
    bool called_itself_last = false;
    for (;;) {
        Frame frame;
        frame.site = &site;
        const signed long long int code =
            run_turn(registry, capsules, functions, site, arguments, self, state, frame);
        if (stops_the_program(code))
            return code;
        const std::vector<std::bitset<16>> &row = registry[site.row];
        if (frame.answered) {
            std::string why;
            if (!value_fits(site.returns, frame.answer, why))
                return raise_at(types_do_not_meet,
                                site.shown + " answers " + shape_written(site.returns) +
                                    ", and what this satellite.return handed back does not fit -- " + why,
                                std::string(), state, row, frame.returned_at);
        }
        if (frame.pending) {
            called_itself_last = true;
            arguments = std::move(frame.arguments);
            continue;
        }
        if (answer != nullptr && answered != nullptr) {
            *answered = frame.answered && !called_itself_last;
            if (*answered)
                *answer = std::move(frame.answer);
            return code;
        }
        if (answer != nullptr) {
            if (!frame.answered || called_itself_last)
                return raise_at(capsule_gave_no_answer,
                                site.shown + "'s answer is used, and it ended without handing one back -- " +
                                    (called_itself_last
                                         ? std::string("its last line calls itself and drops that answer")
                                         : std::string("the way it went reached no satellite.return(...) with a "
                                                       "value")),
                                std::string(), state, row, site.declared_at);
            *answer = std::move(frame.answer);
        }
        return code;
    }
}

} // namespace

signed long long int run_typed_line(const BytecodeRegistry &registry,
                                   const FunctionTable &functions,
                                   MachineState &state)
{
    static const CapsuleTable none;   // a typed line stands alone: there are no capsules around it
    VariableTable variables;          // and no name outlives the line that wrote it, until M6
    Frame frame;                      // nothing to return from: the checker refuses a return here
    return close_files(variables, run_statements(registry, none, functions, 0, 0, variables, state, frame));
}

signed long long int run_capsule_for(const BytecodeRegistry &registry,
                                     const CapsuleTable &capsules,
                                     const FunctionTable &functions,
                                     const CapsuleSite &site,
                                     std::vector<Value> arguments,
                                     const UserDefinedHandle &self,
                                     MachineState &state,
                                     Value *answer)
{
    return run_site(registry, capsules, functions, site, std::move(arguments), self, state, answer);
}

signed long long int run_capsule_on_a_thread(const BytecodeRegistry &registry,
                                            const CapsuleTable &capsules,
                                            const FunctionTable &functions,
                                            const CapsuleSite &site,
                                            std::vector<Value> arguments,
                                            MachineState &state,
                                            Value &answer,
                                            bool &answered)
{
    answered = false;
    return run_site(registry, capsules, functions, site, std::move(arguments), UserDefinedHandle(), state, &answer,
                    &answered);
}

signed long long int run_capsule(const BytecodeRegistry &registry,
                                const CapsuleTable &capsules,
                                const FunctionTable &functions,
                                const std::string &key,
                                std::vector<Value> arguments,
                                MachineState &state)
{
    const CapsuleSite *found = capsules.by_key(key);
    // THE CHECKER HAS ALREADY PROVED THIS, before anything ran: a button whose
    // capsule does not exist is refused with the rest of the program. This is
    // the second reader saying so anyway, because a walker that trusts a name it
    // was handed is a walker that crashes when something else stops checking.
    if (found == nullptr)
        return report_error("satl(run): no capsule named " + capsule_as_written(key), satl_line_not_understood);
    return run_site(registry, capsules, functions, *found, std::move(arguments), UserDefinedHandle(), state, nullptr);
}

signed long long int run_main(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              MachineState &state)
{
    // THE MAIN FILE'S satellite.main, and never an included file's. A file may carry a
    // main of its own to be run by itself, and while the table was one map by name
    // the LAST file read won -- so including such a file ran ITS main instead of the
    // program's.
    const CapsuleSite *main = capsules.main();
    if (main == nullptr)
        return report_error("satl(run): no satellite.main to begin in",
                            satl_file_missing_satellite_main);
    VariableTable variables;   // main's own, and the program's only frame to start
    // THE ARGUMENTS VARIABLE (main_arguments.hpp): main's parameter, when it
    // declares one, is every row satl holds -- `satellite.main()` asks for nothing.
    // It is written `satellite.variable.arguments anything_typed_in_here`, and the
    // older `satellite.container.list<satellite.variable.string> arguments` is the
    // same object: main's parameter is the arguments whatever it was declared as.
    if (!main->parameters.empty()) {
        TypeShape arguments_shape;
        arguments_shape.word = word::code_of(1, 6, 21);
        variables[main->parameters.front().name] =
            Variable{arguments_shape.word, arguments_shape, the_arguments_value(state.arguments, functions)};
    }
    // MAIN'S FRAME HAS NO SITE, as it had no TailCall before one: main calling itself
    // last is not made a loop, and a return anywhere in it still ends the program.
    Frame frame;
    return close_files(variables,
                       run_statements(registry, capsules, functions, main->row, main->body, variables, state, frame));
}

} // namespace satellite004
