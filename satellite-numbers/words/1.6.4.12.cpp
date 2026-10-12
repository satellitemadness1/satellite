// satellite-numbers/words/1.6.4.12.cpp -- satellite.variable.number.modulus(a, b), built into satl
//
// satellite.variable.number.modulus(a, b)   1 6 4 12
// The number modulo x: what is left after dividing by x, with the sign of the number -- the % operator's own
// call, so it answers what % answers, and x of 0 is refused (S401). 003 wrote it modulus(a, b), with the receiver
// as a; as a method it takes ONE argument.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.modulus(x)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// the % operator's: satelliteObject::modulus, which expression.cpp's binary operator asks. NOT
// number_methods.cpp's modulus -- written for this word on 2026-09-16 and never wired, it refused any answer that
// was not whole.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &, ExpressionContext &context)
{
// `.modulus(x)` IS `%` -- number_methods.cpp's header: "n.modulus(3) and n % 3 are the same call".
    // So it is the object model's own modulus, the one the operator calls, and its refusal is the operator's
    // sentence: a call here never answers what `%` would not.
    Value answer;
    std::string why;
    const signed long long int code = receiver.modulus(arguments[0], answer, why);
    if (code != success) {
        context.refuse(code, why);
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_12(LibraryRow *row)
{
    row->name = "satellite.variable.number.modulus(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 12;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
