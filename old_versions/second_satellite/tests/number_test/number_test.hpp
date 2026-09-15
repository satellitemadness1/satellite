#pragma once

// The harness, and the seven sections. Three functions and a counter, no
// framework -- the shape words_test, lexer_test, parser_test, satc_test,
// reporter_test, limits_test and resolve_test already use.
//
// WHAT IS BEING PROVED. PLAN M8's done-when is four clauses: exact
// arbitrary-precision arithmetic runs, negation and abs and the ordering of
// negatives are right, there is no negative zero, and sizeof is inside DESIGN
// §8.2's 40-byte Value budget. The last of those is a static_assert in
// bignum_number.hpp and needs no run-time check; the other three are what the
// sections below decompose into.
//
// THE SUBJECT IS A PORT, WHICH CHANGES WHAT A TEST IS FOR HERE. The first
// satellite's satellite_number worked and was measured, so a suite that only
// re-proved its arithmetic would be checking a transcription. What is NEW at M8
// is the sign: DESIGN §8.1 replaced a significand that carried its own sign with
// an explicit `positive` bool, and every operation that decides a sign was
// rewritten around that. So section_sign is the largest section here, and the
// arithmetic sections concentrate on the cases where a sign and a magnitude
// meet -- opposite-sign addition, a product's sign, the ordering of negatives,
// and every path that can produce a zero.
//
// AND ONE ASSERTION IS ABOUT A BUG THE PORT FIXED rather than about the port
// being faithful. v1's to_integer() refused LLONG_MIN -- its BigInt::fits_ll()
// capped the magnitude at LLONG_MAX, so the one value whose magnitude does not
// fit a signed long long came back as "does not fit" even though v1's own
// constructor accepted it and its to_string() printed it. Measured against v1's
// objects on 2026-08-31 before the claim was written down. An unsigned magnitude
// removes the whole question, and section_sign asserts the round trip.

#include "satellite_number/bignum.hpp"

#include <string>
#include <vector>

namespace number_test {

extern int failures;

void check(bool ok, const std::string &what);

// A number from text, so a fixture reads as the value it is. Fails the suite
// rather than returning a wrong answer quietly -- a test whose fixture does not
// parse is a test asserting about zero.
satellite::Number of(const std::string &text);

// What a number prints as. Every assertion below compares TEXT rather than
// comparing two Numbers, because comparing two Numbers would use the very
// compare() that half of these sections are about.
std::string text_of(const satellite::Number &value);

// A generator with no PCG behind it, so the draw's uniformity can be tested
// against a source whose output is known. The first satellite proved the Bits32
// seam is generator-agnostic exactly this way.
class Counting : public satellite::Bits32 {
public:
    explicit Counting(unsigned first) : next_(first) {}
    unsigned next() override { return next_++; }

private:
    unsigned next_;
};

void section_limbs();       // BigInt: the base-10^9 core, unchanged by M8
void section_sign();        // DESIGN §8.1 -- the milestone's own subject
void section_arithmetic();  // exact where §8.1 promises exact
void section_rounding();    // floor, ceil, round, and what they do to negatives
void section_text();        // parse and to_string, and the notation boundary
void section_exact();       // modulus and the shifts -- DESIGN §8.6's class 1
void section_draw();        // the uniform draw, and satellite_random's consumer

} // namespace number_test
