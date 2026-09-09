#pragma once

// satellite.variable.binary `1 6 5` -- DESIGN §8.5's type. PLAN M19.5.
//
// A RUN OF BITS, AND THE WIDTH IS PART OF THE VALUE. §8.5: "`x0009` is not
// `x9`. That is the whole reason they are not number literals in another
// base." So `b0010` and `b10` are two values, `b0010 == b10` is false, and
// nothing here ever trims a leading zero -- a leading zero is not padding, it
// is a bit the program wrote.
//
// ONE BIT PER BIT, WHICH IS THE AUTHOR'S "just use bools" IN ITS CHEAPEST
// FORM. The bits are `satellite.variable.bool`s, and in this tree that type IS
// the C++ `bool` -- satellite_value/value.hpp's variant names `bool` as its
// second arm and the language has no other. `std::vector<bool>` is the
// bit-packed specialisation, so a `satellite.variable.bool` per bit costs
// exactly one bit. Measured 2026-09-08, one million bits:
//
//     std::vector<bool>            125,000 bytes   <- this
//     one byte per bit           1,000,000 bytes
//     one `Value` per bit       40,000,000 bytes
//
// The third row is what "a vector of satellite.variable.bool" means if each
// one keeps its own type tag, and it is 320x this one for the same bits. The
// author raised the memory question before the type was written and this is
// its answer.
//
// INDEX 0 IS THE LEFTMOST BIT AS WRITTEN, most significant first. That is the
// order `b1000` reads in, and it makes `b1000[0]` the `b1` -- the same 0-based
// counting from the left that DESIGN §8.3 gives a string and that
// evaluator/operations_subscript.cpp gives a list, so a subscript means one
// thing in this language rather than one thing per type.
//
// HEX IS NOT HERE YET AND WILL SHARE THIS BODY WITHOUT DERIVING FROM IT. The
// author's plan, 2026-09-08: a hex value is stored as bits and converted at
// display, because a hex digit is exactly four bits and the round trip is
// exact. When it lands it gets its OWN struct holding a `BitRun`, never a
// `struct HexRun : BitRun` -- a `shared_ptr<const HexRun>` converts implicitly
// to a `shared_ptr<const BitRun>`, so inheritance would let a hex value be
// assigned into a binary arm and silently become one. Composition cannot.

#include "satellite_number/bignum.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::bits {

// A run of bits. The whole value -- there is no sign and no radix field,
// because the radix is which variant arm holds this and the sign is a concept
// a bit run does not have.
struct BitRun {
    std::vector<bool> bits;

    size_t width() const { return bits.size(); }
    bool operator==(const BitRun &other) const { return bits == other.bits; }
};

// The digits of a `b` literal, WITHOUT the leading `b`, as a run. False when a
// character is not `0` or `1`, which the lexer has already ruled out --
// lexer.cpp's bits_radix() requires the whole body to be digits of the radix
// before it will call the token a Bits at all. Checked anyway, because this
// function is the type's constructor and a constructor that trusts its caller
// is a crash waiting on the second caller.
bool parse_binary(std::string_view digits, BitRun &out);

// The bits as they were written -- "1100", no prefix. What `display` and
// `to_string` both go through, so the two can never disagree.
std::string digits_of(const BitRun &run);

// The bits as the language writes them -- "b1100". DESIGN §8.5's spelling, and
// the `b` is what tells a reader of output which type answered, the same job
// the always-printed point does for a float.
std::string text_of(const BitRun &run);

// THE VALUE THE BITS STAND FOR -- `b1100` is 12. Exact and always succeeds:
// every bit run has a whole-number value and there is no width to overflow,
// because §8.1's number is unbounded.
//
// THESE TWO ARE NAMED FOR WHAT THEY COMPUTE AND NOT FOR THE METHODS THEY SIT
// UNDER, which is deliberate and was paid for: they were first written as
// `to_number` and `as_number` to match `1 6 5 1` and `1 6 5 4`, the author
// swapped which satellite verb means which, and a C++ name that mirrors a
// satellite spelling turns that into a silent rewiring instead of a one-line
// change. bits_methods.cpp is the only place the two vocabularies meet.
Number value_of(const BitRun &run);

// THE DIGITS READ AS A DECIMAL NUMBER -- `b1100` is one thousand one hundred.
Number digits_as_number(const BitRun &run);

// The bits packed into bytes, most significant bit first, or false when the
// width is not a multiple of 8.
//
// A FILE IS MADE OF BYTES AND HALF A BYTE CANNOT BE WRITTEN. The alternatives
// were padding with zeros, which invents bits the program did not write, and
// left-aligning in the last byte, which invents the same bits and hides it
// better -- both are DESIGN §1.1's "behind the user's back". So the refusal is
// the answer, and it is one a program can act on: the width is the value's own
// and `width()` `1 6 5 2` says what it is.
bool to_bytes(const BitRun &run, std::string &out);

} // namespace satellite::bits
