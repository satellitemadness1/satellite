// satellite/satellite_variable_infinity/infinity_cases.cpp -- satellite_infinity with
// no interpreter around it, driven by check_infinity.py, which holds every answer to
// infinity_oracle.py (SATELLITE_INFINITY.md: "the value column must match").
//
// ONE COMMAND A LINE ON STDIN, ONE ANSWER A LINE ON STDOUT:
//
//     show V            display(V)
//     compare V W       -1, 0 or 1: the order of V and W
//     number V N        -1, 0 or 1: V against the plain number N (a whole number)
//     negate V          display(-V)
//     unit N            display of the canonical infinity-N, made by N nestings
//     deep N            a chain N exponents deep, walked by every walk: "ok N"
//
// A VALUE IS WRITTEN AS ITS TERM LIST, the oracle's own tuple: `[` then each term as
// `(E C)` then `]`, where E is the exponent -- itself a value -- and C the count as
// decimal text. The zero value, and so the exponent 0, is `[]`. check_infinity.py
// writes values this way straight from the oracle's normal form, so the harness never
// has to put a list in order: what it reads is already what a value holds.
//
//     5           [([] 5)]
//     infinity    [([([] 1)] 1)]
//     (inf, -500) [([([] 1)] 1)([] -500)]

#include "satellite_infinity.hpp"

#include "../machine/machine_codes.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace satellite004;

namespace {

void skip_blanks(const std::string &text, std::size_t &at)
{
    while (at < text.size() && text[at] == ' ') ++at;
}

// ONE VALUE FROM `at`, WITHOUT RECURSING, for the same reason the walks do not: the
// deep cases here are the point. Each `[` opens a list of terms on the stack; each
// `]` closes one, and a closed list is either the answer or the exponent of the term
// its parent is reading, whose count and `)` come next.
bool read_value(const std::string &text, std::size_t &at, InfinityHandle &out)
{
    std::vector<std::vector<infinity_term>> open;
    skip_blanks(text, at);
    if (at >= text.size() || text[at] != '[') return false;
    ++at;
    open.emplace_back();
    for (;;) {
        skip_blanks(text, at);
        if (at >= text.size()) return false;
        if (text[at] == '(') {
            ++at;
            skip_blanks(text, at);
            if (at >= text.size() || text[at] != '[') return false;
            ++at;
            open.emplace_back();
            continue;
        }
        if (text[at] != ']') return false;
        ++at;
        InfinityHandle closed = satellite_infinity::make(std::move(open.back()), satellite_number(128ull));
        open.pop_back();
        if (open.empty()) {
            out = std::move(closed);
            return true;
        }
        skip_blanks(text, at);
        const std::size_t from = at;
        while (at < text.size() && text[at] != ')' && text[at] != ' ') ++at;
        infinity_count count;
        if (infinity_count::from_text(text.substr(from, at - from), count) != success) return false;
        skip_blanks(text, at);
        if (at >= text.size() || text[at] != ')') return false;
        ++at;
        open.back().push_back(infinity_term{std::move(closed), std::move(count)});
    }
}

bool read_count(const std::string &text, std::size_t &at, satellite_number &out)
{
    skip_blanks(text, at);
    const std::size_t from = at;
    while (at < text.size() && text[at] != ' ') ++at;
    std::size_t bad = 0;
    return satellite_number::from_text(text.substr(from, at - from), out, bad) == success;
}

InfinityHandle unit(unsigned long long int rank)
{
    InfinityHandle made = satellite_infinity::infinity(satellite_number(128ull));
    for (unsigned long long int k = 0; k < rank; ++k)
        made = satellite_infinity::make({infinity_term{made, infinity_count::of_whole(satellite_number(1ull))}},
                                        satellite_number(128ull));
    return made;
}

// A CHAIN THAT IS NOT A CANONICAL UNIT ANYWHERE, so no short cut can answer for it:
// (infinity, +1) at the bottom, and `infinity ** that` on top, `depth` times over.
// Built twice, independently, so compare() has no shared node to stop at and must
// walk the whole way down both.
InfinityHandle chain(unsigned long long int depth)
{
    const infinity_count one = infinity_count::of_whole(satellite_number(1ull));
    InfinityHandle made = satellite_infinity::make(
        {infinity_term{satellite_infinity::one(), one}, infinity_term{nullptr, one}}, satellite_number(128ull));
    for (unsigned long long int k = 0; k < depth; ++k)
        made = satellite_infinity::make({infinity_term{made, one}}, satellite_number(128ull));
    return made;
}

std::string deep(unsigned long long int depth)
{
    InfinityHandle a = chain(depth), b = chain(depth), shallower = chain(depth - 1);
    if (satellite_infinity::compare(a.get(), b.get()) != 0) return "FAIL two equal chains compare unequal";
    if (satellite_infinity::compare(a.get(), shallower.get()) != 1) return "FAIL the deeper chain is not larger";
    if (satellite_infinity::compare(shallower.get(), a.get()) != -1) return "FAIL the shallower chain is not smaller";
    const std::string shown = satellite_infinity::display(a.get());
    // (infinity^( ... (infinity, +1) ... )): "(infinity^" once a level, then the bottom and the closers.
    const std::size_t wanted = depth * 10 + std::string("(infinity, +1)").size() + depth;
    if (shown.size() != wanted) return "FAIL the display is " + std::to_string(shown.size()) + " long";
    a.reset(); b.reset(); shallower.reset();          // the destructor's own walk, 3 x depth deep
    return "ok " + std::to_string(depth);
}

} // namespace

int main()
{
    std::string line;
    int bad = 0;
    while (std::getline(std::cin, line)) {
        const std::size_t space = line.find(' ');
        const std::string command = line.substr(0, space);
        std::size_t at = space == std::string::npos ? line.size() : space;
        InfinityHandle v, w;
        satellite_number n;
        if (command == "show" && read_value(line, at, v)) {
            std::cout << satellite_infinity::display(v.get()) << '\n';
        } else if (command == "compare" && read_value(line, at, v) && read_value(line, at, w)) {
            std::cout << satellite_infinity::compare(v.get(), w.get()) << '\n';
        } else if (command == "number" && read_value(line, at, v) && read_count(line, at, n)) {
            std::cout << satellite_infinity::compare(v.get(), n) << '\n';
        } else if (command == "negate" && read_value(line, at, v)) {
            std::cout << satellite_infinity::display(satellite_infinity::negated(v.get()).get()) << '\n';
        } else if (command == "unit" && read_count(line, at, n) && n.fits_one_limb() && !n.negative()) {
            std::cout << satellite_infinity::display(unit(n.limb(0)).get()) << '\n';
        } else if (command == "deep" && read_count(line, at, n) && n.fits_one_limb() && !n.negative() &&
                   !n.is_zero()) {
            std::cout << deep(n.limb(0)) << '\n';
        } else {
            std::cout << "UNREAD " << line << '\n';
            ++bad;
        }
    }
    return bad == 0 ? 0 : 1;
}
