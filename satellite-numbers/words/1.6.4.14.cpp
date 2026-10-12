// satellite-numbers/words/1.6.4.14.cpp -- satellite.variable.number.sqrt(a), built into satl
//
// satellite.variable.number.sqrt(a)   1 6 4 14
// Numbered, and NOT BUILT: an exact whole square root is a piece of work of its own (Newton's method over the
// limbs), and 003 DESIGN 8.6 puts sqrt in the class that must round, which a whole number cannot do. Refused by
// its number rather than left out, so `n.satellite.sqrt()` says what is missing.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.sqrt()` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// number_methods.cpp's -- written 2026-09-16 for this word and never wired (in no make file, called by nothing)
// -- moved here whole the day that file was deleted.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &, Value *, const TypeShape *, const std::vector<Value> &, bool, const std::string &name,
             ExpressionContext &context)
{
    // THE CHECKER SAYS IT FIRST, on a name declared a number (number_method_check); this is for a value it
    // could not see -- a literal in brackets, a chain, an item of a list.
    context.refuse(not_built_yet, name + ".satellite.sqrt " + number_method_not_built(token::sqrt_token));
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_14(LibraryRow *row)
{
    row->name = "satellite.variable.number.sqrt(a)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 14;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
