// satellite-numbers/words/1.6.17.2.cpp -- satellite.variable.infinity.power_of(x), built into satl
//
// satellite.variable.infinity.power_of(x)   1 6 17 2
// The infinity to the power of x -- NOT BUILT: SATELLITE_INFINITY.md INF-4 builds a whole power, and INF-5 an
// infinite one. .to_the_power_of and .power are its second spellings, and Part 5 calls it "new, and distinct
// from" the number's power (1 6 4 10), which a name holding a plain number runs.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author, 2026-10-11: "we still need to get the word satellite onto
// everything, before we do anything... .pointer, numbers floats, everything builtin needs the word satellite
// still"): `i.satellite.power_of(x)` reaches the function below through the function table --
// bytecode/number_calls.cpp's run_number_number -- and the code in it is what expression.cpp's infinity guard
// said until that day, copied here: the word is numbered and not built, and says which milestone builds it
// (infinity_calls.cpp's infinity_method_not_built, which the checker asks too).
// NUMBERED THE SAME DAY, from 1 6 17 1: the infinity had no words of its own under satellite.variable.infinity
// (INF-1 made the tokens), and its name may hold a plain number (SATELLITE_INFINITY.md Q24) -- so an infinity's
// period is its own words and the number's (method_periods.hpp).

#include "../number_row.hpp"
#include "../../satellite/bytecode/number_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/infinity_calls.hpp"

namespace {

using namespace satellite004;

Value method(const Value &, Value *, const TypeShape *, const std::vector<Value> &, bool, const std::string &name,
             ExpressionContext &context)
{
    // THE CHECKER SAYS IT FIRST, on a name declared an infinity (program_check.cpp's method_judged); this is
    // for a value it could not see -- satellite.infinity() itself, a chain, an item of a list.
    context.refuse(not_built_yet, name + ".satellite.power_of " + infinity_method_not_built(token::power_of_token));
    return Value();
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_17_2(LibraryRow *row)
{
    row->name = "satellite.variable.infinity.power_of(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 17;
    row->numbers[3] = 2;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
