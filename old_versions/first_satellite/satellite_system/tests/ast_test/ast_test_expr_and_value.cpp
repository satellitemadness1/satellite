// The one check that guards the decision the whole tree is built around: a
// syntax node and a runtime value are two different variants, and this is what
// it costs to fold them together.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. These
// checks build no trees, so the only thing they take from ast_test.hpp is the
// failure counter they report into, which ast_test.cpp defines.

#include "ast_test.hpp"

#include "satellite_value/value.hpp"   // only to assert the two variants stayed independent

void ast_test_expr_and_value_stay_separate()
{
    // ---- Expr and Value stay separate ------------------------------------
    // This is what the split buys, and it is the reason the expression kinds
    // did not join Value's variant. A syntax node is much larger than a
    // runtime value, so folding the two together would have grown every
    // number and every list element in the language to match the largest
    // syntax node — turning a list of a million numbers from 38 MB into
    // 91 MB to carry fields no value ever reads.
    check(sizeof(Value) < sizeof(Expr),
          "folding Expr into Value would grow every runtime value");
}
