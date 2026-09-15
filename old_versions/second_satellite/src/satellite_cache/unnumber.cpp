// Numbers back into words -- the mirror of satellite_cache/paths.cpp and
// write.cpp, which are the two files that turn words into numbers. See
// satellite_cache/read.cpp for the reading order this is one step of, and
// satellite_cache/cache.hpp for why a `.satc` is read by the ordinary parser
// rather than by a grammar of its own.
//
// EVERYTHING THAT UNDOES A NUMBER IS IN THIS ONE FILE, which is the same
// property write_internal.hpp claims for the forward direction and it is worth
// the same sentence: a second place that decided what `#1.5.4` stands for would
// be a second place the numbering's spelling lives, and the two would drift
// apart on the day a row grew an argument list.
//
// IT IS A TEXT PASS AND NOT A PARSE, deliberately. The body of a `.satc` is a
// satellite program with some of its words spelled as digits, so putting the
// words back gives a satellite program -- and the lexer and the parser that
// read a source read that. A version of this that built tree nodes would be the
// second grammar the module exists to avoid.

#include "satellite_cache/cache.hpp"

#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::cache {

namespace {

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

// The node a `.satc`'s number names, or NodeId::NONE.
//
// number_text() RUN BACKWARDS, AND NOT A SECOND TABLE. The forward direction
// spells a node's position among its parent's children (words_numbers.hpp), so
// the reverse is that walk done by comparing rather than by appending -- which
// keeps the numbering the only place a number's meaning lives. A lookup table
// built from the same data would be a second copy of it, and the copy would be
// the one that was right on the day it was written.
//
// 0 IS A REAL SEGMENT. WORD_NUMBERS §1.3 makes the bare call shape position 0 --
// `satellite.include()` is 1 1 0 -- so a reader that skipped zero, or treated it
// as a terminator, would refuse a form the writer emits.
words::NodeId node_of(const std::vector<uint32_t> &segments)
{
    words::NodeId at = words::NodeId::NONE;
    for (const uint32_t want : segments) {
        words::PathId found = words::kNoPath;
        for (words::PathId c = words::first_child(at); c != words::kNoPath;
             c = words::next_sibling(c))
            if (words::number_of(static_cast<words::NodeId>(c)) == want) {
                found = c;
                break;
            }
        if (found == words::kNoPath)
            return words::NodeId::NONE;
        at = static_cast<words::NodeId>(found);
    }
    return at;
}

// What a source would have written where the file wrote this number.
//
// THE ROW'S ARGUMENT LIST IS PART OF THE PATH AND USUALLY NOT PART OF THE TEXT,
// which is the one subtlety in the whole reverse direction. §2.2 spells 1 5 4 as
// `satellite.console.input(prompt, target)`, but the file says `#1.5.4("name: ",
// x)` -- the row names the SHAPE and the file carries the VALUES, so writing the
// spelling back out would give `input(prompt, target)("name: ", x)`, which is a
// program the language does not have.
//
// TWO ROW SHAPES KEEP THEIR PARENTHESES AND BOTH ARE THE WRITER'S MIRROR. At
// arity 0 the row IS the empty call -- 1 5 2 already says `input()` and the file
// wrote no parentheses after it -- and an absorber's argument is the reserved
// word, which the number names and the file also did not write (§5.1 step 3).
// write.cpp's numbered_chain() decides those same three cases in the same order,
// and the two functions are wrong together or not at all.
std::string path_source(words::NodeId id)
{
    std::string out = words::path_text(id);
    const std::string_view args = words::arguments_of(id);
    if (arity_of(args) > 0 && !is_absorber(args))
        out.resize(out.size() - args.size());
    return out;
}

// How much of what path_source() writes is WORDS, which is what a node's anchor
// token can end at. cache.hpp's Mark is what this is for.
//
// THE ARGUMENT LIST IS NEVER PART OF IT, WHICHEVER WAY path_source() WENT.
// Above, an arity-0 row and an absorber KEEP their parentheses and every other
// shape has them stripped -- but the characters that differ are parentheses in
// both cases, and no node is anchored at one. So the words end at the same
// offset either way and this needs no branch at all.
//
// IT HAD ONE, AND IT WAS INVERTED. Written as `arity > 0 && !absorber ? whole :
// whole - args.size()` it returned the FULL length for exactly the rows
// path_source() shortens, so every call shape's mark landed 16 characters past
// the word it belonged to and matched no node. The effect was silent and
// one-sided: `satellite.console.display` skipped and
// `satellite.console.input(prompt, target)` did not, and every number was still
// correct because the walk is the fallback. Found on 2026-08-31 by a mutation
// that asked which HALF of the skip a test was covering --
// tests/resolve_test/cache.cpp carries that, and MILESTONES/M7.md §7 carries
// why the totals could not have caught it.
size_t word_length(words::NodeId id)
{
    return words::path_text(id).size() - words::arguments_of(id).size();
}

// WHERE THE SELECTOR BEFORE AN OPTION ENDS -- M19.6, and it is a LOOKBEHIND
// because the file writes `my_list.sort(0#down)` and the Mark has to land on
// `sort`.
//
// THE MARK GOES ON THE SELECTOR AND NOT ON THE OPTION, WHICH IS WHAT MAKES IT
// REACHABLE. numbers.cpp's path_of() looks a Mark up by `ast_.token_of(anchored)
// .end` with `anchored = ast_[call].a` -- the Member node, anchored at the word
// `sort`. A Mark sitting where `0#down` ends would key on the String node
// instead and no selector would ever find it, so the offset has to be walked
// back to the word the number is ABOUT.
//
// IT IS SAFE BECAUSE THE WRITER EMITS `name(` WITH NOTHING BETWEEN THEM.
// write_expressions.cpp's Call arm says `expr(n.a)`, `"("`, then the arguments,
// so an option at position 0 is always preceded by exactly one `(` and the
// selector's last character. Anything else in that position is a `.satc` this
// build did not write, and returning npos refuses it rather than guessing.
size_t selector_ends_before(const std::string &into)
{
    size_t at = into.size();
    if (at == 0 || into[at - 1] != '(')
        return std::string::npos;
    at--; // the `(` itself; the word ends where it begins
    const size_t ends = at;
    while (at > 0 && is_option_char(into[at - 1]))
        at--;
    return at < ends ? ends : std::string::npos;
}

} // namespace

bool unnumber(const std::string &body, std::string &into,
              errors::Diagnostic &why, Marks *marks, Folded *folded)
{
    into.clear();
    into.reserve(body.size() * 2);
    if (marks != nullptr)
        marks->clear();
    if (folded != nullptr)
        folded->clear();

    for (size_t at = 0; at < body.size();) {
        const char c = body[at];

        // A STRING IS COPIED AND NEVER SCANNED, because `display("#1.5.1")` is
        // a program that prints five characters and not a program that prints
        // `satellite.console.display`. The escape rule is the lexer's own
        // (lexer.cpp): a backslash takes the next character with it unless that
        // character is the newline, because a literal may not cross a line.
        if (c == '"') {
            into += c;
            at++;
            while (at < body.size() && body[at] != '"' && body[at] != '\n') {
                const bool escaped = body[at] == '\\' && at + 1 < body.size() &&
                                     body[at + 1] != '\n';
                into.append(body, at, escaped ? 2 : 1);
                at += escaped ? 2 : 1;
            }

            // THE CLOSING QUOTE IS TAKEN HERE, AND NOT TAKING IT INVERTED THE
            // WHOLE PASS. Without this the loop above stops ON the quote and
            // `continue` hands it back to the top, which reads it as an OPENING
            // one -- so the characters AFTER a string were copied through as
            // string contents and the characters inside the NEXT string were
            // read as code. A `#` in the first stretch reached the parser as
            // itself, and the parser has no rule for one (SATC §1.1.1 is the
            // sentence that makes that certain), so the file was refused as
            // malformed by the check that exists to catch a corrupt cache.
            //
            // IT NEEDED A PATH AFTER A STRING ON ONE LINE, which is why it
            // survived a year of round-trip tests: the inner loop also stops at
            // a newline -- a literal may not cross one -- so every line begins
            // in the right state and only a line with both on it went wrong.
            // `#1.6.6 b = #1.8.5("no_such") == #1.17.1` is the shape, and
            // example/persistence.satl was the only acceptance program that
            // had it. section_reading()'s fixpoint runs over four programs and
            // none of them do; literals_are_not_scanned() wrote its string
            // LAST on the line, which is the one position that hides this.
            // tests/satc_test/reading.cpp now writes one that does not.
            //
            // A `.satc` NOBODY CAN READ IS STILL SAFE AND THAT IS WHY IT WAS
            // QUIET. §4's "a missing, stale or unreadable `.satc` is never an
            // error" held the whole time: the program ran from its source and
            // was correct. What it cost was the cache -- such a program wrote a
            // file on every single run and read one on none of them.
            if (at < body.size() && body[at] == '"')
                into += body[at++];
            continue;
        }

        // §1.1's COMMENT COLUMN IS "NEVER TRUSTED -- the numbers are the file",
        // so it is copied through untouched rather than read. The lexer drops it
        // a moment later (DESIGN §5.6); what this skip buys is that a `#` a
        // person typed into a comment cannot make a well-formed file malformed.
        if (c == '/' && at + 1 < body.size() && body[at + 1] == '/') {
            while (at < body.size() && body[at] != '\n')
                into += body[at++];
            continue;
        }

        // SATC.md §3's THIRD KIND OF TOKEN -- M19.6's `0#down`, and it is read
        // BEFORE the `#` scan below because its second character is one.
        //
        // A LETTER AFTER THE MARK AND NOT A DIGIT, which is the whole of what
        // keeps it apart from a literal `0` butted against a path: `#` then
        // digits is a number and `#` then a letter is an option, and no option
        // in the numbering begins with a digit because they are all read off a
        // `<word>_<option>` spelling. AND NO IDENTIFIER CHARACTER BEFORE IT,
        // because this pass copies bytes rather than tokens -- without that
        // test the `0` of a literal `10` would start an option token if a path
        // ever followed it with nothing in between.
        if (c == '0' && at + 2 < body.size() && body[at + 1] == kPathMark &&
            (is_option_char(body[at + 2]) &&
             !(body[at + 2] >= '0' && body[at + 2] <= '9')) &&
            (at == 0 || !is_option_char(body[at - 1]))) {
            size_t scan = at + 2;
            while (scan < body.size() && is_option_char(body[scan]))
                scan++;

            // THE MARK GOES ON THE SELECTOR, WHICH IS BEHIND US. cache.hpp's
            // `Folded` says why the entry is a bare offset and why the word
            // itself is not carried: the string is about to go back into the
            // text, so the resolver reads it off the tree.
            const size_t ends = selector_ends_before(into);
            if (ends == std::string::npos) {
                why = errors::make<errors::Code::SATC_OPTION_WITHOUT_A_CALL>(
                    errors::kNowhere, body.substr(at, scan - at));
                return false;
            }
            if (folded != nullptr)
                folded->push_back(static_cast<uint32_t>(ends));

            // WHAT GOES BACK IS THE STRING THE PROGRAM WROTE, so the text the
            // parser reads is the text a source would have given it and the
            // tree is the same tree. §3's "literals stay literal" is kept on
            // both sides of the round trip -- the file spells the option as a
            // token, and the moment it stops being a file it is a string again.
            into += '"';
            into.append(body, at + 2, scan - (at + 2));
            into += '"';
            at = scan;
            continue;
        }

        if (c != kPathMark) {
            into += c;
            at++;
            continue;
        }

        // THE SEGMENTS ARE COLLECTED THE WAY THE LEXER COLLECTS A FLOAT, and
        // that is not a coincidence -- a dot joins only when a digit follows it.
        // `#1.14.counter` is `satellite.library` and then a user's name, and a
        // scan that took every dot and digit would swallow the trailing dot and
        // come away with an empty segment. The mark says where a number starts;
        // the same rule the lexer already has says where it ends.
        size_t scan = at + 1;
        std::vector<uint32_t> segments;
        while (scan < body.size() && is_digit(body[scan])) {
            uint32_t value = 0;
            while (scan < body.size() && is_digit(body[scan]))
                value = value * 10 + static_cast<uint32_t>(body[scan++] - '0');
            segments.push_back(value);
            if (scan + 1 < body.size() && body[scan] == '.' &&
                is_digit(body[scan + 1]))
                scan++;
            else
                break;
        }

        if (segments.empty()) {
            why = errors::make<errors::Code::SATC_MARK_WITHOUT_A_NUMBER>(
                errors::kNowhere, kPathMark);
            return false;
        }

        const words::NodeId id = node_of(segments);
        if (id == words::NodeId::NONE) {
            why = errors::make<errors::Code::SATC_UNKNOWN_PATH>(
                errors::kNowhere, body.substr(at, scan - at));
            return false;
        }

        // THE RECORD M7 READS, AND IT IS TAKEN HERE BECAUSE HERE IS THE ONLY
        // PLACE THAT KNOWS. A moment later this is a satellite program with no
        // numbers in it at all -- which is the whole point of the pass, and is
        // exactly why MILESTONES/M4.5.md §5 says a warm hit does strictly more
        // work than reading the source until resolve is given somewhere to
        // learn it from. cache.hpp's Mark is why the offset is the END of the
        // words rather than their start.
        if (marks != nullptr)
            marks->push_back({static_cast<uint32_t>(into.size() + word_length(id)),
                              static_cast<words::PathId>(id)});
        into += path_source(id);
        at = scan;
    }
    return true;
}

} // namespace satellite::cache
