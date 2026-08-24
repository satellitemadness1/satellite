// The machine ops -- the second id space, which restarts at 1 and is not the
// word space. Part of the format_test binary; the harness is declared in
// format_test.hpp.
//
// The last checks below are the ones that matter most: op 7 and word 7 share a
// number, deliberately, and reading one against the other's table is exactly the
// bug the kind tag on every operand exists to catch. They are asserted as an
// agreement rather than left as a coincidence, so that a future edit which
// "fixes" the overlap has to argue with a test.

#include "format_test.hpp"

using namespace satellite::format;

// --- machine ops ------------------------------------------------------------

void test_ops()
{
    check(static_cast<uint64_t>(Op::OP_MOVE) == 1, "the op space restarts at 1");
    // A count, from the data. `OP_RETURN == 9` was labelled "nine machine ops"
    // and checked neither the count nor the density — the ids being dense from 1
    // is now a static_assert in the header, which is what makes this a count.
    check(sizeof(detail::kOpIds) / sizeof(uint64_t) == 9, "nine machine ops");
    check(static_cast<uint64_t>(Op::OP_RETURN) == 9, "return is the last of them");

    check_str(name(Op::OP_CALL_METHOD), "call_method", "op 8 names itself");

    // The operand counts §17.4 and §17.5 specify.
    check(arity(Op::OP_MOVE) == 2, "move takes dst, src");
    check(arity(Op::OP_JUMP) == 1, "jump takes a target");
    check(arity(Op::OP_CALL) == 4, "call takes capsule, first arg, count, dst");
    check(arity(Op::OP_CALL_METHOD) == 5,
          "call_method takes recv, selector, first arg, count, dst");

    // Both polarities exist from the start, because && and || cannot lower to
    // the and/or selectors — a call evaluates its arguments and a short circuit
    // must not (§17.5).
    check(arity(Op::OP_JUMP_IF_TRUE) == 2 && arity(Op::OP_JUMP_IF_FALSE) == 2,
          "a branch of each polarity, both taking test and target");

    // The two spaces are separate. Op 7 and word 7 are unrelated, and reading one
    // against the other's table is the bug the kind tag exists to catch.
    check(static_cast<uint64_t>(Op::OP_CALL) == static_cast<uint64_t>(Word::DISPLAY),
          "op 7 and word 7 share a number, as separate spaces are entitled to");
    check_str(name(Op::OP_CALL), "call", "...and op 7 is call");
    check_str(name(Word::DISPLAY), "display", "...while word 7 is display");
}
