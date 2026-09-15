// The three operations that never round -- modulus and the two shifts.
//
// DESIGN §8.6 SORTS EVERY OPERATION INTO THREE CLASSES AND THIS SECTION IS THE
// FIRST ONE: "exact and bounded, never rounds, never grows ... safe in a loop
// forever." That classification is why these three are here at all. All three
// spent 2026-08-31 assigned to M15 on the grounds that they could not be
// finished before the rounding rule was chosen, and none of them was waiting on
// it -- §8.6 answers modulus in full, and a shift by a power of two terminates
// in both directions because 2 divides 10. MILESTONES/M8.md §6 is the review of
// how that was found.
//
// SO WHAT EVERY ASSERTION BELOW IS REALLY CHECKING IS THAT NOTHING ROUNDED. A
// suite that only checked values would pass over an implementation that ran the
// answer through divide() and kept 34 digits, because 34 digits is enough for
// every small fixture. The fixtures here are chosen so that a rounding
// implementation gives a DIFFERENT answer: a quotient that does not terminate,
// a shift whose exact answer is 70 digits long, and DESIGN §8.6's own worked
// example, which is the one a binary float gets wrong.

#include "number_test.hpp"

namespace number_test {

using satellite::Number;

void section_exact()
{
    // --- modulus, `1 6 4 12` ------------------------------------------------
    //
    // `a % b` is `a - b * trunc(a/b)`, truncated and not floored. sign.cpp
    // holds the sign half of that -- -7 % 2, 7 % -2, and -8 % 4 among the
    // twenty-two ways to reach a zero -- and this holds the exactness half.
    check(text_of(Number::modulo(of("7"), of("2"))) == "1", "7 % 2 is 1");
    check(text_of(Number::modulo(of("8"), of("4"))) == "0",
          "and a modulus that divides evenly is zero");

    // DESIGN §8.6'S OWN WORKED EXAMPLE, WHICH IS ALSO THE ONE A double GETS
    // WRONG. §8.6 writes it out: trunc(7.5 / 2.1) = 3, 3 * 2.1 = 6.3,
    // 7.5 - 6.3 = 1.2. In IEEE 754 that last subtraction is 1.2000000000000002,
    // and satl printing 1.2 is the whole of what "exact decimal" buys.
    check(text_of(Number::modulo(of("7.5"), of("2.1"))) == "1.2",
          "7.5 % 2.1 is exactly 1.2 -- DESIGN §8.6's worked example, and the "
          "answer a binary float cannot give");
    check(text_of(Number::modulo(of("-7.5"), of("2.1"))) == "-1.2",
          "and it takes the sign of the DIVIDEND either side of zero");

    // THE ASSERTION THAT SAYS THE ROUNDING RULE WAS NEVER ITS BLOCKER. 1 / 0.3
    // does not terminate -- it is 3.333... -- so an implementation that reached
    // for divide() would have to round it, and rounding 3.333... to any budget
    // then multiplying back gives something that is not 0.1. The quotient is
    // only ever wanted as an integer, so the tail is discarded before it exists.
    check(text_of(Number::modulo(of("1"), of("0.3"))) == "0.1",
          "1 % 0.3 is exactly 0.1, over a quotient that does not terminate -- "
          "which is why modulus never consults division_digits");

    // And once past the small form, where the quotient is a bignum divmod.
    check(text_of(Number::modulo(of("100000000000000000000000000001"),
                                 of("7"))) == "6",
          "a boxed dividend is the same operation one representation up");

    // --- shift_left, `1 6 4 1` ----------------------------------------------
    //
    // A SHIFT IS MULTIPLICATION BY A POWER OF TWO, decided 2026-08-31 and
    // written into DESIGN §5.5. On an integer that is the C meaning exactly.
    check(text_of(Number::shift_left(of("1"), 10)) == "1024",
          "1 shifted left ten places is 1024, which is what a bit shift means");
    check(text_of(Number::shift_left(of("3"), 0)) == "3",
          "a shift by zero is the value, and does not go near the multiply");
    check(text_of(Number::shift_left(of("-3"), 2)) == "-12",
          "and a shift keeps the sign it was given, on both sides of zero");
    check(text_of(Number::shift_left(of("0"), 5)) == "0",
          "zero shifts to zero");
    check(Number::shift_left(of("0"), 5).positive(),
          "and it is a POSITIVE zero, because there is no other kind");

    // THE QUESTION §4.1 OF MILESTONES/M8.md SAID NEITHER CANDIDATE MEANING
    // ANSWERED: what happens to a receiver with a fractional part. Nothing
    // special happens. It is a multiplication, so the fraction rides along and
    // the answer is exact.
    check(text_of(Number::shift_left(of("7.5"), 1)) == "15",
          "7.5 shifted left once is exactly 15 -- a fractional receiver needs "
          "no rule of its own");
    check(text_of(Number::shift_left(of("0.125"), 3)) == "1",
          "and three places takes 0.125 to exactly 1");

    // Past the small form: 2^64 does not fit an unsigned long long significand,
    // so this is the exact answer arriving through the BigInt path.
    check(text_of(Number::shift_left(of("1"), 64)) == "18446744073709551616",
          "1 shifted left 64 places is 2^64 exactly, one past what the small "
          "form holds");
    check(Number::shift_left(of("1"), 64).payload_bytes() != 0,
          "and it boxed to say it");

    // --- shift_right, `1 6 4 11` --------------------------------------------
    //
    // DIVISION BY A POWER OF TWO, AND IT TERMINATES BECAUSE 2 DIVIDES 10. That
    // is the whole reason this direction needs no rounding rule either, and it
    // is what makes these two a pair rather than one safe operation and one
    // open question.
    check(text_of(Number::shift_right(of("1024"), 10)) == "1",
          "1024 shifted right ten places is 1");
    check(text_of(Number::shift_right(of("7.5"), 1)) == "3.75",
          "7.5 shifted right once is exactly 3.75");
    check(text_of(Number::shift_right(of("-8"), 1)) == "-4",
          "and the sign survives this direction too");
    check(text_of(Number::shift_right(of("1"), 10)) == "0.0009765625",
          "1 shifted right ten places is 2^-10, written out in full");

    // THE FIXTURE THAT WOULD CATCH A shift_right BUILT OUT OF divide(). 2^-100
    // is 70 significant digits, so a division carrying the default 34 would
    // round it -- and this asserts the count as well as the value, because a
    // rounded answer is the right digits with the tail missing.
    const Number small_shift = Number::shift_right(of("1"), 100);
    check(small_shift.digit_count() == 70,
          "1 shifted right a hundred places keeps all 70 of its digits, twice "
          "the default division budget, because nothing here divides");
    check(text_of(Number::shift_left(small_shift, 100)) == "1",
          "and shifting it back returns exactly 1, which is the round trip a "
          "rounded answer cannot make");

    // The two directions against each other on a value that is neither a power
    // of two nor an integer, which is where an exponent adjustment done the
    // wrong way round survives every fixture above.
    check(text_of(Number::shift_right(of("12.34"), 5)) == "0.385625",
          "12.34 shifted right five places is exact to six decimal places");
    check(text_of(Number::shift_left(of("0.385625"), 5)) == "12.34",
          "and shifting that back is the value it came from");
}

} // namespace number_test
