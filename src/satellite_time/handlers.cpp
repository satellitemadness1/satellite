// `satellite.time.now` `1 9 1` and `satellite.time.sleep(n)` `1 9 3`, behind
// the table. See satellite_time/handlers.hpp for the two rows that are not
// here and why.

#include "satellite_time/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_time/time.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::time {

namespace {

// `satellite.time.now` `1 9 1` -- a fact, asked fresh, never cached. It is
// property-shaped on purpose: compile_expressions' module-constant arm makes
// a language path read without being called a zero-argument dispatch, so
// `display(satellite.time.now)` reaches this row with no parentheses
// anywhere. Two asks in a row answer two different values, which is done-when
// clause 6 and the whole reason the representation is an integer.
bool time_now(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::instant(now_nanoseconds());
    return true;
}

// `satellite.time.sleep(n)` `1 9 3` -- n IS SECONDS, WHOLE OR FRACTIONAL,
// the author's decision of 2026-09-04: one unit always, so there is nothing
// at a call site to misread, and 0.09 is an exact Number so the fraction
// costs no float and no M15. v1's `100ms` literal died with the special case
// that consumed it (PLAN §7).
//
// `sleep(n, unit)` `1 9 4` IS THE SAME WAIT WITH n COUNTED IN ANOTHER UNIT --
// the author, 2026-09-12, `sleep(90, "ms")`. The unit is how many decimal
// places n sits above a nanosecond, so the conversion below stays one exact
// multiplication by a power of ten and no unit costs a float either.
bool sleep_for(eval::Machine &m, const Value *arguments, int32_t places,
               Value *answer);

bool time_sleep(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    return sleep_for(m, arguments, 9, answer);
}

bool time_sleep_unit(eval::Machine &m, const Value *arguments, uint32_t,
                     Value *answer)
{
    static const struct {
        const char *word;
        int32_t places;
    } kUnits[] = {{"s", 9}, {"ms", 6}, {"us", 3}, {"ns", 0}};

    const Str *text = std::get_if<Str>(&arguments[1]);
    if (text == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            "a unit word (\"s\", \"ms\", \"us\" or \"ns\")",
            type_name(arguments[1])));
        return false;
    }
    const std::string word = *text ? decode(**text) : std::string();
    for (const auto &unit : kUnits)
        if (word == unit.word)
            return sleep_for(m, arguments, unit.places, answer);
    m.refuse(errors::make<errors::Code::TIME_NO_SUCH_UNIT>(
        m.span_of(m.here()), "\"" + word + "\""));
    return false;
}

bool sleep_for(eval::Machine &m, const Value *arguments, int32_t places,
               Value *answer)
{
    const Number *seconds = std::get_if<Number>(&arguments[0]);
    if (seconds == nullptr) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            "a number of seconds", type_name(arguments[0])));
        return false;
    }

    // NEGATIVE IS NOT A LENGTH, AND NEITHER IS FOREVER. The count the timer
    // runs on is int64 nanoseconds, so a request the multiplication cannot
    // carry -- about 292 years -- is refused in the same sentence as a
    // negative one: S0920 is about the LENGTH, where S0713 above is about the
    // type. Sub-nanosecond fractions floor away first, v1's pace rule: a
    // fraction of a nanosecond is not a wait anybody can observe.
    long long count = 0;
    const Number in_nanoseconds =
        Number::mul(*seconds, Number::from_small(true, 1, places)).floor();
    if (seconds->is_negative() || !in_nanoseconds.to_integer(count)) {
        m.refuse(errors::make<errors::Code::TIME_NOT_A_LENGTH>(
            m.span_of(m.here()), seconds->to_string()));
        return false;
    }

    // The wait happens on the walking thread and the printer keeps printing
    // through it -- its own thread, DESIGN §10.1 -- so a display queued before
    // a sleep reaches the terminal during it, not after. An interrupted sleep
    // answers the same nothing a finished one does: the flag is set, and the
    // walk stops itself at the next statement boundary exactly as DESIGN
    // §10.2 promises. Nothing here reports S0730 -- that is the boundary's
    // sentence to say, with its caret under the statement that did not run.
    (void)sleep_nanoseconds(count, m.policy().interrupted);

    *answer = Value::nothing();
    return true;
}

} // namespace

void install_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();

    // Neither row binds a receiver: `time` is a namespace, not a value --
    // M10's display note, third and fourth rows along.
    table.install(static_cast<words::PathId>(NodeId::TIME_NOW),
                  eval::Handler{time_now, false, 0, "M13"});
    table.install(static_cast<words::PathId>(NodeId::TIME_SLEEP),
                  eval::Handler{time_sleep, false, 1, "M13"});
    table.install(static_cast<words::PathId>(NodeId::TIME_SLEEP_UNIT),
                  eval::Handler{time_sleep_unit, false, 2, "M13"});
}

} // namespace satellite::time
