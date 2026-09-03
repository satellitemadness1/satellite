// A value as characters, and the six live codes answered. See
// satellite_value/render.hpp.
//
// THIS FILE IS WHAT PLAN §6.1 CALLED "REPLACING SIX LINES WITH SIX CALLS", and
// the six calls are in live_values() below. They are here rather than in
// satellite_string/ because the lexer calls decode() on every token's text and
// `--unparse` prints that text back: a live decode inside the alphabet would
// write this machine's thread count into the source of any program containing
// "\threads". satellite_string.hpp carries the argument at length.

#include "satellite_value/render.hpp"

#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"
#include "system_facts/facts.hpp"

#include <string>

namespace satellite {

namespace {

// The table satellite_string::decode() fills its live codes from, in code
// order 95..100.
//
// ONE FUNCTION AND NOT SIX CALL SITES, so that the ORDER is written once. The
// array is indexed by `code - SAT_LINUX_HOME` and nothing in the type system
// says which entry is which -- the assert in satellite_string.hpp catches a
// code being added and this ordering is what a reader checks against DESIGN
// §5's table. Every entry is filled, including the ones a given string will not
// use, because finding out which are used means walking the string first and
// the walk is what this feeds.
Live live_values()
{
    Live live;
    live[SAT_LINUX_HOME - SAT_LINUX_HOME] = facts::home_dir();
    live[SAT_LINUX_USERNAME - SAT_LINUX_HOME] = facts::username();
    live[SAT_THREADS - SAT_LINUX_HOME] = std::to_string(facts::hardware_threads());
    live[SAT_MEM_TOTAL_MB - SAT_LINUX_HOME] = std::to_string(facts::mem_total_mb());
    live[SAT_MEM_USED_MB - SAT_LINUX_HOME] = std::to_string(facts::mem_used_mb());
    live[SAT_CWD - SAT_LINUX_HOME] = facts::cwd();
    return live;
}

// Whether this string has a live code in it at all.
//
// THE CHEAP HALF OF A DECISION THAT IS OTHERWISE FOUR /proc READS AND A
// getpwuid PER RENDER. Almost no string holds one -- they are reachable only
// through encode()'s backslash names -- so a scan of the codes buys the common
// case out of the machine entirely. The scan is over 16-bit units already in
// cache and the comparison is a range test, which is why this is worth a
// function rather than a comment saying it would be.
bool has_a_live_code(const SatString &text)
{
    for (SatChar c : text)
        if (c >= SAT_LINUX_HOME && c <= SAT_CWD)
            return true;
    return false;
}

} // namespace

std::string live_text(const SatString &text)
{
    if (!has_a_live_code(text))
        return decode(text);
    return decode(text, live_values());
}

std::string text_of(const Value &value)
{
    if (const bool *flag = std::get_if<bool>(&value)) {
        // THE LANGUAGE'S OWN SPELLING AND NOT C++'s. DESIGN §8's table writes
        // a bool as `satellite.bool.true` / `.false`, and M11 is the milestone
        // that gives those two paths a value to answer with. What a person sees
        // printed is the word, because the path is how it is WRITTEN and this
        // is how it READS -- the same split the number type has between
        // `satellite.variable.number` and `3`.
        return *flag ? "true" : "false";
    }

    if (const Number *number = std::get_if<Number>(&value))
        return number->to_string();

    if (const Str *text = std::get_if<Str>(&value))
        return *text ? live_text(**text) : std::string();

    // THE RUNTIME PRINTS AS THE WORD THE PROGRAM WROTE. DESIGN §8's table gives
    // `satellite` a row of its own and §3 calls it "the singleton runtime
    // object, not a zero sentinel"; a value that printed as blank or as
    // `nothing` would be that sentinel arriving through the renderer, which is
    // the one thing the row exists to deny.
    if (value.is_runtime())
        return "satellite";

    // NOTHING PRINTS AS A WORD RATHER THAN AS AN EMPTY LINE, which is DESIGN
    // §1.1's rule about never doing anything behind the user's back applied to
    // the smallest possible case: a capsule that returned nothing and a capsule
    // that returned the empty string must not print the same thing. M12 is
    // where a program gains a way to ASK which it has.
    return "nothing";
}

} // namespace satellite
