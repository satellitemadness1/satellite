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

} // namespace

bool unnumber(const std::string &body, std::string &into,
              errors::Diagnostic &why)
{
    into.clear();
    into.reserve(body.size() * 2);

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

        into += path_source(id);
        at = scan;
    }
    return true;
}

} // namespace satellite::cache
