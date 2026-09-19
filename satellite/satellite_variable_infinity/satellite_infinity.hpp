#pragma once
// satellite/satellite_variable_infinity/satellite_infinity.hpp --
// `satellite.variable.infinity` `1 6 17`, and every number above it, as ONE value
// (SATELLITE_INFINITY.md, INF-2).
//
// A VALUE IS A LIST OF TERMS, LARGEST FIRST. A term is a count and a unit, and the
// unit is infinity raised to an exponent -- which is itself a value, so the type is
// recursive:
//
//     5                      [(0, 5)]              5, a plain number
//     infinity               [(1, 1)]              (infinity)
//     infinity - 500         [(1, 1), (0, -500)]   (infinity, -500): the author's
//                                                  REGISTER is every term after the first
//     infinity.power_of(inf) [(infinity, 1)]       (infinity-1), a power
//
// The first term decides what the value IS, and its sign. The reference model is
// infinity_oracle.py beside this file, and check_infinity.py holds this code to it:
// the text display() answers must be the oracle's, and the order compare() answers
// must be the oracle's, for every value the spec's tables hold.
//
// IMMUTABLE AND SHARED. The arm of satelliteObject is a handle to a const value
// (satellite_object.hpp, arm 12), so `b = a` copies a pointer, and an exponent two
// values have in common is one exponent. Nothing on a mutating path can reach into
// one -- an operation answers a NEW value, the way `+` does.
//
// EVERY WALK KEEPS ITS OWN STACK. A loop of `x = inf.power_of(x)` nests exponents
// one deeper every pass, as deep as memory allows (Part 4), so compare, display and
// the destructor each keep their stack on the heap. None has a depth bound: a bound
// is never the fix (satellite has no limits).

#include "../satellite_variable_number/satellite_number.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

// A COUNT: AN EXACT DECIMAL, IN THE FLOAT'S SHAPE (Part 3, Part 10). A sign, the
// whole part and the fraction -- M20's decided float, "a bool and two
// satellite_numbers" -- so `(0.5 infinity)` and `(0.25 infinity, +1.5)` need no float
// arm. 12.05 is (12, 5, 2 places) and 12.5 is (12, 5, 1 place).
//
// THE FRACTION CARRIES ITS OWN COUNT OF PLACES rather than being padded out to
// arguments.infinity, and its last digit is never 0. So one number has one spelling
// here -- equal counts are equal field by field -- and a row changed between two
// values never changes what either of them means. arguments.infinity is the most
// places a count may have (INF-3 refuses more); it is not how a count is stored.
struct infinity_count {
    bool negative = false;          // never true of 0
    satellite_number whole;         // never negative: the sign is `negative`
    satellite_number fraction;      // the digits after the point, read as a whole number
    std::size_t places = 0;         // how many digits `fraction` stands for; 0 when there are none

    static infinity_count of_whole(const satellite_number &value);
    // "2", "-0.5", "12.05" -- digits, at most one point with a digit on both sides of
    // it, and a leading minus. Trailing zeros after the point are dropped. Answers
    // success, or int_error (3) with `out` untouched.
    static signed long long int from_text(const std::string &text, infinity_count &out);

    bool is_zero() const { return whole.is_zero() && places == 0; }
    bool is_one() const { return !negative && places == 0 && whole == satellite_number(1ull); }
    int sign() const { return is_zero() ? 0 : (negative ? -1 : 1); }
    infinity_count negated() const;

    // EVERY PLACE, NEVER CUT: a count is exact by construction, so its text is too
    // (Q43). magnitude_text() leaves the sign off, for a term that prints its own.
    std::string text() const;
    std::string magnitude_text() const;

    static int compare(const infinity_count &left, const infinity_count &right);   // -1, 0 or 1
};

class satellite_infinity;
using InfinityHandle = std::shared_ptr<const satellite_infinity>;

struct infinity_term {
    InfinityHandle exponent;        // nullptr IS THE EXPONENT 0: a plain number, attached
    infinity_count count;           // never 0
};

class satellite_infinity {
public:
    // LARGEST FIRST, no two terms with equal exponents, no count of 0: the normal
    // form, which is what makes one number one list and lets compare() walk two lists
    // side by side. The zero value is no terms at all, and a handle to it is nullptr.
    std::vector<infinity_term> terms;

    // THE NINES WIDTH (Part 9): arguments.infinity when the value was made, and what
    // `.resize(n)` will change (INF-7). Only `.nines()` reads it -- no answer ever
    // depends on it -- and an exponent's is never read at all.
    satellite_number nines_width;

    // IS THIS THE CANONICAL infinity-k, a tower of k + 1 infinities, and which k.
    // Worked out ONCE, when the value is made, from its exponent's own answer: the
    // exponent is always made first, so the question costs nothing and never walks.
    // unit_rank 0 is (infinity) itself, 1 is (infinity-1), the power.
    bool a_unit = false;
    satellite_number unit_rank;

    satellite_infinity() = default;
    satellite_infinity(const satellite_infinity &) = default;
    satellite_infinity(satellite_infinity &&) = default;
    satellite_infinity &operator=(const satellite_infinity &) = default;
    satellite_infinity &operator=(satellite_infinity &&) = default;
    ~satellite_infinity();          // releases a deep chain of exponents without recursing

    // THE ONE WAY A VALUE IS MADE: the terms, already in normal form, and the nines
    // width. Works out a_unit. Answers nullptr for no terms -- the zero value.
    static InfinityHandle make(std::vector<infinity_term> terms, satellite_number nines_width);

    // satellite.infinity() -- (infinity), the list [(1, 1)].
    static InfinityHandle infinity(const satellite_number &nines_width);
    // A plain number as a value of the family, for comparing one with the other and as
    // an exponent. nullptr for 0.
    static InfinityHandle of_number(const satellite_number &value);
    static const InfinityHandle &one();     // the exponent 1, shared

    // Every count's sign turned over: `-infinity` is `infinity * -1` (Q20). The
    // exponents are shared, not copied.
    static InfinityHandle negated(const satellite_infinity *value);

    // THE ORDER: the sign of the first term of left - right (Part 3). -1, 0 or 1;
    // nullptr is 0. A positive infinity is larger than every number, a negative one
    // smaller, whatever is attached.
    static int compare(const satellite_infinity *left, const satellite_infinity *right);
    static int compare(const satellite_infinity *left, const satellite_number &right);

    // THE DISPLAY (Part 8): one number is one set of parentheses -- `(infinity)`,
    // `(2 infinity)`, `(infinity, -500)`, `(infinity-1)`, `(infinity^(infinity, +1))`
    // -- and a plain number shows bare. nullptr shows 0.
    static std::string display(const satellite_infinity *value);

    // No term has an exponent: this is a plain number (only ever an exponent, or what
    // is left when an infinity goes away -- which INF-3 hands back as a number).
    bool is_plain() const;
    // The exponent 1: one term, a plain count of exactly 1.
    bool is_one() const;
};

} // namespace satellite004
