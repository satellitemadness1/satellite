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
// INDEX 0 IS THE RIGHTMOST BIT -- the least significant, the 1s place -- so
// `b0001[0]` is `b1`. THE AUTHOR'S, 2026-09-09, AND IT REVERSES WHAT THIS
// COMMENT SAID FOR A DAY: until then this file and DESIGN §8.5 both said
// leftmost, on the ground that a string and a list count from the left and a
// subscript should mean one thing in this language.
//
// THE AUTHOR TOOK THE HARDWARE CONVENTION INSTEAD, and the cost is real and is
// written here rather than argued away: a subscript now counts one way on a
// string or a list and the other way on a bit run. `[-1]` reaches the far end
// on all of them, as it does everywhere else.
//
// NOTHING BELOW IMPLEMENTS IT. `[` is an OPERATOR, costs no path number, and is
// a later milestone's -- these lines exist so that milestone starts from the
// decision instead of re-taking it, which is the whole reason DESIGN §8.5 keeps
// the rest of the operator set written down and unbuilt.
//
// HEX SHARES THIS BODY WITHOUT DERIVING FROM IT -- `HexRun` at the bottom of
// this file, M19.5's second half, landed 2026-09-09. The author's plan,
// 2026-09-08: a hex value is stored as bits and converted at display, because
// a hex digit is exactly four bits and the round trip is exact. It got its OWN
// struct holding a `BitRun`, never a `struct HexRun : BitRun` -- a
// `shared_ptr<const HexRun>` converts implicitly to a `shared_ptr<const
// BitRun>`, so inheritance would let a hex value be assigned into a binary arm
// and silently become one. Composition cannot.

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

// ---------------------------------------------------------------------------

// A RUN OF HEX DIGITS -- `satellite.variable.hex` `1 6 11`, DESIGN §8.5's
// other radix and the second half of M19.5.
//
// IT HOLDS A `BitRun` AND DOES NOT DERIVE FROM ONE, which the note at the top
// of this file called before either existed. Under inheritance a
// `shared_ptr<const HexRun>` converts implicitly to `shared_ptr<const BitRun>`,
// so a hex value could be assigned into value.hpp's binary arm and silently
// become a binary one -- a conversion in a language DESIGN §8 says has none.
// Composition cannot be converted by accident, and that is the whole reason
// for the extra `.bits` in every expression below.
//
// STORED AS BITS AND CONVERTED AT DISPLAY -- the author's, 2026-09-08 -- and
// what makes it cost nothing is arithmetic rather than taste: one hex digit is
// exactly four bits, 2^4 being 16, so neither direction rounds and neither
// representation is the "real" one. `to_number()` reads the same value off
// either, which is the fact the whole type leans on.
//
// THE WIDTH IS ALWAYS A MULTIPLE OF FOUR, BY CONSTRUCTION. A hex value is
// built from DIGITS and each contributes four bits, so `digits()` never
// rounds and never lies. That invariant is exactly what makes `to_binary()`
// total and `to_hex()` partial, and the two are one function apart below.
struct HexRun {
    BitRun bits;

    // WIDTH IS BITS, DIGITS ARE DIGITS, AND BOTH GOT A ROW because the author
    // asked for both on 2026-09-09: `x00FF` is sixteen wide and four digits.
    // `width()` `1 6 11 2` means the same thing it means on a bit run -- how
    // many bits -- which is what lets `write(x)`'s multiple-of-eight rule stay
    // ONE rule across the two types instead of reading as multiple-of-two here.
    size_t width() const { return bits.width(); }
    size_t digits() const { return bits.width() / 4; }
    bool operator==(const HexRun &other) const { return bits == other.bits; }
};

// The digits of an `x` literal, WITHOUT the leading `x`, as a run. False when a
// character is not a hex digit -- which the lexer has already ruled out, and
// which is checked here for parse_binary's reason one type up.
//
// BOTH CASES ARE ACCEPTED AND NEITHER IS KEPT. lexer_chars.hpp takes `x00ff`
// and `x00FF` alike "because the width is what carries meaning in §8.5 and
// case does not" -- and since the value is the BITS, the case is not stored at
// all. The two literals are one value, `x00ff == x00FF` is true, and
// `digits_of` below picks the spelling back.
bool parse_hex(std::string_view digits, HexRun &out);

// The digits as the language writes them back -- "00FF", no prefix.
//
// UPPERCASE, AND IT IS A CHOICE THIS FUNCTION MAKES ALONE. The case a program
// wrote is gone by the time anything gets here, so `display` cannot print what
// was written the way it can for a bit run; it can only be consistent. Upper
// is what DESIGN §8.5 writes in every example it has -- `x00FF`, `x0009` --
// so the language prints hex the way its own specification spells it.
std::string digits_of(const HexRun &run);

// The digits as the language writes them -- "x00FF". The `x` does the job the
// `b` does one type up and the float's always-printed point does a third: it
// tells a reader of output which type answered.
std::string text_of(const HexRun &run);

// A bit run as a hex run, or false when the width is not a multiple of four.
//
// THE REFUSAL IS `to_bytes`'s REFUSAL ONE RADIX OVER AND FOR ITS REASON. Four
// bits are one digit and three bits are no digits at all; padding to four
// invents a bit the program never wrote and left-aligning invents the same bit
// while hiding it better. So `b101.to_hex()` `1 6 5 6` refuses and says the
// width, and there is no direction here in which something is quietly made up.
//
// THE OTHER DIRECTION NEEDS NO FUNCTION AND CANNOT FAIL: a `HexRun`'s `.bits`
// IS the answer to `to_binary()` `1 6 11 6`, always, by the multiple-of-four
// invariant above.
bool to_hex(const BitRun &run, HexRun &out);

} // namespace satellite::bits
