// satellite-numbers/words/1.6.4.10.cpp -- satellite.variable.number.power(a, b), built into satl
//
// satellite.variable.number.power(a, b)   1 6 4 10
// The number to the power of x -- the ^ operator's own call, so it answers what ^ answers: 2 to the power of -1
// is 0.5, a float. 003 wrote it power(a, b), with the receiver as a; as a method it takes ONE argument.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `n.satellite.power(x)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number, which has refused a wrong count first -- and the code in it is
// the ^ operator's: satelliteObject::power, which expression.cpp's binary operator asks. NOT number_methods.cpp's
// power -- written for this word on 2026-09-16 and never wired, it refused any answer that was not whole, where ^
// has answered 2 ^ -1 as 0.5 since SATELLITE_INFINITY.md FLT-2.
// The number is 003's, kept when 004 numbered its words (2026-09-14); the word had no code of its own until now.

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &, ExpressionContext &context)
{
// `.power(x)` IS `^` (the author, 2026-09-16: "the to the power of can be ^ and an alias you have to take
// number_name.power() so .power() becomes something you can call on that object"). .power_of and
// .to_the_power_of are the lexer's second spellings of it (SATELLITE_INFINITY.md, messages five and nine).
    // So it is the object model's own power, the one the operator calls, and its refusal is the operator's
    // sentence: a call here never answers what `^` would not.
    Value answer;
    std::string why;
    const signed long long int code = receiver.power(arguments[0], answer, why);
    if (code != success) {
        context.refuse(code, why);
        return Value();
    }
    return answer;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_4_10(LibraryRow *row)
{
    row->name = "satellite.variable.number.power(a, b)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 10;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
