// satellite-numbers/words/1.6.3.2.1.cpp -- satellite.variable.time.time.convert(unit), built into satl
//
// satellite.variable.time.time.convert(unit)   1 6 3 2 1
// The same length of time, shown in the unit asked for (the author, 2026-10-26: ".satellite.time.convert("seconds")
// or "ns" or "nanoseconds" or "ms" or "milliseconds" or "s" will convert that time to another time"): 1500ms
// .satellite.time.convert("s") is 1.5s. Every nanosecond is kept -- a time is whole nanoseconds, and its unit is only
// how it is shown -- so 1s == 1s.satellite.time.convert("ms") is true. Nothing between the brackets is the nanosecond,
// as it is for satellite.time.now().
//
// A NEW TIME, AND THE NAME KEEPS ITS OWN (TIME_PLAN.md Section 7, reading 3), as a list's `.satellite.reverse()`
// answers a reversed list and leaves the list alone. To change the name it is given back:
// waited = waited.satellite.time.convert("ns").
//
// ONE WORD OF TWO PARTS, laid out as the window's window.section: the lexer reads time.convert as one code after
// `.satellite.` (REGISTRY.satellite's time_convert_token), and `time` is a part, satellite.variable.time.time 1 6 3 2,
// which runs nothing and is refused written alone (time_calls.hpp's a_time_part). The unit is read where
// satellite.time.now reads its own, bytecode/time_calls.cpp, so the two words take the same six spellings and say
// the same sentence.
//
// It reaches the function below through the function table -- bytecode/scalar_calls.cpp's run_scalar_number. The
// checker said its brackets, its count and a literal unit before anything ran (program_check.cpp's chain_periods);
// a unit worked out is judged here, when its line runs. The sentences are spelled only when one is said: `name` and
// the word go to time_calls.cpp as they are, so a convert that is right builds no text.

#include "../number_row.hpp"
#include "../../satellite/bytecode/time_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace {

using namespace satellite004;

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &arguments, bool had_parentheses,
             const std::string &name, ExpressionContext &context)
{
    static constexpr const char *kWord = ".satellite.time.convert";
    std::string why;
    if (const signed long long int refused = unit_call_refused(had_parentheses, arguments.size(), name, kWord, why);
        refused != success) {
        context.refuse(refused, why);
        return Value();
    }
    satellite_time converted = *receiver.as_time();
    if (!unit_given(arguments, name, kWord, converted.unit, context))
        return Value();
    return Value::of_time(std::move(converted));
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_3_2_1(LibraryRow *row)
{
    row->name = "satellite.variable.time.time.convert(unit)";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 3;
    row->numbers[3] = 2;
    row->numbers[4] = 1;
    row->depth = 5;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
