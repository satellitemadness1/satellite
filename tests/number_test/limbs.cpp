// BigInt -- the base-10^9 core, which M8 changed in exactly two places.
//
// A SMALLER SECTION THAN IT LOOKS LIKE IT SHOULD BE, ON PURPOSE. This class
// came across from the first satellite unchanged apart from fits_ll()/to_ll()
// becoming fits_u64()/to_u64(), because a magnitude was ALREADY unsigned there
// -- DESIGN §8.1's sign change is about the layer above. Re-proving schoolbook
// multiplication would be checking a transcription, so what is checked here is
// the properties the layer above leans on and the pair that changed.
//
// IT IS REACHED THROUGH Number, not directly. BigInt's operations are public
// but its constructor takes limbs nobody outside the module builds, so the
// honest way to drive it is through the values Number makes -- which is also
// the only way anything in satl will ever reach it.

#include "number_test.hpp"

#include <string>

namespace number_test {

using satellite::BigInt;
using satellite::Number;

void section_limbs()
{
    // --- the limb boundary --------------------------------------------------
    //
    // Base 10^9 means nine decimal digits per limb, so the interesting inputs
    // are the ones that land exactly on a boundary. A padding bug in
    // to_digits() -- dropping a limb's leading zeros -- concatenates 1 and
    // 000000002 as 12, and only shows on a value with a zero-heavy middle limb.
    check(text_of(of("1000000002")) == "1000000002",
          "1000000002 survives the limb split -- its middle is zeros and a "
          "to_digits() that dropped a limb's padding would print 12");
    check(text_of(of("1000000000000000002")) == "1000000000000000002",
          "and again across three limbs");
    check(text_of(Number::add(of("999999999"), of("1"))) == "1000000000",
          "a carry out of the top limb");
    check(text_of(Number::sub(of("1000000000"), of("1"))) == "999999999",
          "a borrow into the top limb, which trim() then shortens");

    // --- digit_count is about the value, not the storage --------------------
    //
    // bignum_number.hpp's own note: 100 normalises to 1e2, and a count that
    // read the significand would answer 1. It is the difference between
    // reporting the number and reporting how it happens to be held.
    check(of("100").digit_count() == 3, "100 has three digits");
    check(of("1230").digit_count() == 4, "1230 has four");
    check(of("12.5").digit_count() == 3, "12.5 has three");
    check(of("0.001").digit_count() == 1,
          "0.001 has one -- a leading zero is padding and not a digit");
    check(of("0").digit_count() == 1, "zero has one, because `0` is a digit");
    check(of("-100").digit_count() == 3, "a sign is not a digit");
    check(of("1e40").digit_count() == 41, "1e40 is one digit and forty zeros");

    // --- what a magnitude costs to hold -------------------------------------
    //
    // DESIGN §8.2's promise is that the small case never allocates. This is
    // where that is a measured fact rather than an intention.
    check(of("0").payload_bytes() == 0, "zero allocates nothing");
    check(of("1").payload_bytes() == 0, "a loop counter allocates nothing");
    check(of("-2.5").payload_bytes() == 0, "a small fraction allocates nothing");
    check(Number(LLONG_MAX).payload_bytes() == 0,
          "the largest value the small form holds still allocates nothing");
    check(of("1e40").payload_bytes() == 0,
          "1e40 allocates nothing either -- it is a significand of 1 and an "
          "exponent, which is what an exact decimal buys over a bignum");
    check(of("1234567890123456789012345678901234567890").payload_bytes() != 0,
          "forty distinct digits do not fit the small form and box");
    check(of("1234567890123456789012345678901234567890").payload_bytes() %
                  sizeof(unsigned) ==
              0,
          "a boxed magnitude is billed by the limb, four bytes each");

    // --- the pair that changed ----------------------------------------------
    //
    // fits_u64()/to_u64() are reachable only through make(), which is what
    // decides whether a computed value comes back small or boxed. A fits_u64()
    // that was still capped at LLONG_MAX would box every magnitude between
    // 2^63 and 2^64 -- correct answers, silently allocating.
    const Number just_under = of("18446744073709551615");   // ULLONG_MAX
    check(just_under.payload_bytes() == 0,
          "ULLONG_MAX fits the small form -- v1's fits_ll() stopped at "
          "LLONG_MAX and would have boxed it");
    check(text_of(just_under) == "18446744073709551615",
          "and comes back out of it unchanged");
    check(of("18446744073709551616").payload_bytes() != 0,
          "one past ULLONG_MAX boxes, which is where the small form ends");

    // divmod's contract, through the division that uses it.
    check(text_of(Number::divide(of("1000000000000000000000"), of("1000000000"),
                                 34)) == "1000000000000",
          "a division that is a limb shift comes out exact");
}

} // namespace number_test
