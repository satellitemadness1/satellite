// The depths a `.satc` may be written at. See tests/satc_test/satc_test.hpp
// for the harness.
//
// THE WRITER IS THE THIRD OF M8.5's FOUR WALKERS. DESIGN §7.5: no walker may
// use the C++ stack for a depth the user's program controls, and `satl --satc`
// died with signal 11 at 20,000 nested brackets before this milestone
// (`SCRATCH.md/NO_LIMITS.md` §2.4). write_internal.hpp says the writer took
// unparse.cpp's shape when that one stopped recursing; this is where that is
// checked rather than asserted.
//
// AND THIS BINARY DOES NOT RAISE ITS OWN STACK either -- the argument is
// tests/parser_test/depth.cpp's, one suite over, and it is what makes 8 MiB the
// number these checks run against.
//
// THE DEPTH HERE IS SMALLER THAN THE PARSER'S AND THE REASON IS THE OUTPUT.
// The writer indents four spaces per level like the printer it is a copy of, so
// a nested BLOCK costs the square of its depth in spaces -- which is a fact
// about how much answer there is to hold and not about the walk. The nesting
// that costs nothing extra is an EXPRESSION's, and that is what the big number
// is spent on here.

#include "satc_test.hpp"

#include <string>

namespace satc_test {

namespace {

std::string repeated(const std::string &text, int times)
{
    std::string out;
    out.reserve(text.size() * static_cast<size_t>(times));
    for (int i = 0; i < times; i++)
        out += text;
    return out;
}

// How many times one character appears, which is how a nest is counted without
// walking anything.
size_t occurrences(const std::string &text, char what)
{
    size_t seen = 0;
    for (const char c : text)
        if (c == what)
            seen++;
    return seen;
}

} // namespace

void section_depth()
{
    constexpr int kDeep = 100000;

    // NESTED BRACKETS, the shape §2.4 measured this command dying on.
    //
    // ONE FEWER BRACKET COMES OUT THAN WENT IN, and that is the printer being
    // right rather than off by one: brackets are put back from PRECEDENCE and
    // not from memory, and the outermost pair of a nest is the one pair that
    // changes nothing. `(1 + (1 + 1))` prints as `1 + (1 + 1)`, which
    // unparse.cpp does too and tests/parser_test's round trip is what says the
    // two agree.
    {
        const std::string written =
            statements("satellite.variable.number n = " + repeated("(1 + ", kDeep) +
                       "1" + repeated(")", kDeep));
        check(occurrences(written, '(') == static_cast<size_t>(kDeep) - 1,
              "a `.satc` is written for 100,000 nested brackets");
        check(occurrences(written, ')') == static_cast<size_t>(kDeep) - 1,
              "and every one of them is closed");
    }

    // NESTED CALLS, which reach the writer through its argument list rather
    // than through its bracket -- and every one of them is a call on a user's
    // name, so SATC §3.1's rule leaves all 100,000 as text.
    {
        const std::string written =
            statements("satellite.variable.number n = " + repeated("f(", kDeep) +
                       "1" + repeated(")", kDeep));
        check(occurrences(written, '(') == static_cast<size_t>(kDeep),
              "a `.satc` is written for 100,000 nested calls");
    }

    // NESTED BLOCKS, at a depth whose OUTPUT is a size a machine can hold: the
    // indent is four spaces a level, so 2,000 levels is already eight million
    // characters of nothing but space. The walk is what is being checked and it
    // is the same walk at 2,000 as at 100,000.
    {
        constexpr int deep = 2000;
        const std::string written =
            statements(repeated("{\n", deep) +
                       "satellite.variable.number x = 1\n" + repeated("}\n", deep));
        check(occurrences(written, '{') == static_cast<size_t>(deep),
              "a `.satc` is written for 2,000 nested blocks");
        check(occurrences(written, '}') == static_cast<size_t>(deep),
              "and every one of them is closed");
    }

    // THE COMMENT COLUMN AT DEPTH, which is the one thing this writer has that
    // the printer next door does not. A note is made where its number is
    // printed and flushed at the end of the line it is on -- so a hundred
    // numbered types on one line come out as a hundred names in that line's
    // comment, in the order they were written.
    {
        constexpr int wide = 100;
        std::string source;
        for (int i = 0; i < wide; i++)
            source += "satellite.variable.number n" + std::to_string(i) + " = 1\n";
        const std::string written = statements(source);
        check(occurrences(written, '/') == static_cast<size_t>(wide) * 2,
              "every one of a hundred lines carries its own comment column");
    }
}

} // namespace satc_test
