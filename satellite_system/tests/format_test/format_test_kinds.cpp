// The kind space -- the four-bit tag every operand carries, and the half of the
// format test that guards it. Part of the format_test binary; the harness it
// uses (check, check_str, the failure counter) is declared in format_test.hpp.
//
// This section is first because everything after it reads a table that is only
// meaningful once the tag space is intact: a kind that has collided with
// another, or one that has outgrown its four bits, makes the registry and arity
// checks that follow answers to the wrong question.

#include "format_test.hpp"

using namespace satellite::format;

// --- the kind space ---------------------------------------------------------

void test_kinds()
{
    // §17's table, which is now the only place a kind is assigned. These eight
    // are asserted individually rather than by count, because the collision this
    // file exists to prevent was two DIFFERENT things claiming one number, and a
    // count would have been satisfied by that.
    check(static_cast<uint64_t>(Kind::NAME_CODE) == 0, "kind 0 is the name code");
    check(static_cast<uint64_t>(Kind::IMMEDIATE) == 1, "kind 1 is an immediate");
    check(static_cast<uint64_t>(Kind::POOL_REF) == 2, "kind 2 is a pool ref");
    check(static_cast<uint64_t>(Kind::REGISTER) == 3, "kind 3 is a register");
    check(static_cast<uint64_t>(Kind::SPACESUIT_ID) == 4, "kind 4 is a spacesuit id (§17.2)");
    check(static_cast<uint64_t>(Kind::CAPSULE_ID) == 5, "kind 5 is a capsule id (§17.2)");
    check(static_cast<uint64_t>(Kind::MACHINE_OP) == 6, "kind 6 is a machine op (§17.5)");
    check(static_cast<uint64_t>(Kind::DURATION) == 7, "kind 7 is a duration (§9)");

    // The collision itself, stated as the thing it is: two spaces, two tags.
    check(Kind::SPACESUIT_ID != Kind::MACHINE_OP,
          "a spacesuit id and a machine op are different kinds");

    check_str(name(Kind::MACHINE_OP), "machine op", "kind 6 names itself");
    check_str(name(Kind::SPACESUIT_ID), "spacesuit id", "kind 4 names itself");

    // The distinction kind 7 was added for: an operand that IS a number and an
    // operand that is a length of time encode differently, so a decoder can
    // tell display(100) from display(100ms).
    check(Kind::DURATION != Kind::IMMEDIATE,
          "a duration and an immediate are different kinds");
    check_str(name(Kind::DURATION), "duration", "kind 7 names itself");

    // Four bits. A kind past 15 does not survive the encoding at all, so the
    // ceiling is not a style preference.
    //
    // Over EVERY row, not over one named enumerator. This check read
    // `Kind::MACHINE_OP <= 15` under this same message, which is true forever and
    // says nothing about a row added later — the run-time half of a header assert
    // that was counting rows instead of bounding ids. Both were wrong the same
    // way, which is why the wrong one survived review of the other.
    for (size_t i = 0; i < sizeof(detail::kKindIds) / sizeof(uint64_t); i++)
        check(detail::kKindIds[i] <= 15, "every kind fits in four bits");
}
