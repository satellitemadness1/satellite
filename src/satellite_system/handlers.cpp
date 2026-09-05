// The four dials, read and retuned. See satellite_system/handlers.hpp for
// why this is a module and what proves the mechanism it rides on.
//
// PLAN §4.5.3, COMPLETED AT LAST: "the file seeds the namespace at startup
// and the namespace is what everything reads afterwards". The seeding is
// policy_from_the_limits() -- the config file, read once -- and the
// namespace's run-time half is the machine's Policy: every read below
// answers from it, every write below stores into it, and the next division
// or rounding simply observes the new count. Nothing here re-reads a file
// and nothing is process-wide, so M22's many-programs future inherits a dial
// per machine rather than a fight over one.

#include "satellite_system/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "machine_limits/limits.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::system {

namespace {

// --- the reads -- module constants, answering the machine's own Policy -----

bool read_division_digits(eval::Machine &m, const Value *, uint32_t,
                          Value *answer)
{
    *answer = Value::number(Number(
        static_cast<long long>(m.policy().division_digits)));
    return true;
}

bool read_float_digits(eval::Machine &m, const Value *, uint32_t,
                       Value *answer)
{
    *answer = Value::number(Number(
        static_cast<long long>(m.policy().float_digits)));
    return true;
}

bool read_max_depth(eval::Machine &m, const Value *, uint32_t, Value *answer)
{
    // BYTES, AND ALREADY RESOLVED: unset meant the machine at seed time
    // (limits::max_depth_bytes' fallback), so what a program reads is the
    // ceiling actually being enforced, never a zero standing in for a story.
    *answer = Value::number(Number::from_u64(m.policy().max_depth));
    return true;
}

bool read_min_free_mb(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    // THE ONE DIAL WHOSE HOME IS NOT THE POLICY: the watchdog's floor, read
    // from where the watchdog reads it. UNSET ANSWERS NOTHING -- M12's
    // askable state, and the honest one: an unset floor means the machine's
    // free memory is not watched at all, and a 0 here would be a number
    // claiming it is.
    const limits::Dial &dial = limits::held().dial(limits::DialId::MinFreeMb);
    *answer = dial.set ? Value::number(Number::from_u64(dial.value))
                       : Value::nothing();
    return true;
}

// --- the writes -- the retune rows ------------------------------------------

// A dial takes a whole number a machine word holds, or the row says which of
// those it is not -- S0713, the same shape scalars' shift_count uses, because
// at run time the caret is the statement's and the file-and-line answers
// S0807/S0808 give belong to the config file.
bool dial_count(eval::Machine &m, const Value &value, const char *dial,
                unsigned long long least, unsigned long long most,
                unsigned long long *out)
{
    const Number *count = std::get_if<Number>(&value);
    long long wide = 0;
    if (count == nullptr || !count->is_integer() ||
        !count->to_integer(wide) || wide < 0 ||
        static_cast<unsigned long long>(wide) < least ||
        static_cast<unsigned long long>(wide) > most) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), dial,
            "a whole number a machine word holds",
            count == nullptr ? type_name(value)
                             : "`" + count->to_string() + "`"));
        return false;
    }
    *out = static_cast<unsigned long long>(wide);
    return true;
}

bool retune_division_digits(eval::Machine &m, const Value *arguments,
                            uint32_t, Value *)
{
    // The floor is 1 for Number::divide's reason -- a zero count keeps no
    // digits and has nothing to answer -- and the ceiling is the word the
    // count is carried in. The same pair S0807/S0808 hold the config file to.
    unsigned long long count = 0;
    if (!dial_count(m, arguments[0], "division_digits",
                    limits::kDivisionDigitsLeast,
                    limits::kDivisionDigitsMost, &count))
        return false;
    m.retune_division_digits(static_cast<unsigned>(count));
    return true;
}

bool retune_float_digits(eval::Machine &m, const Value *arguments, uint32_t,
                         Value *)
{
    // INT_MAX and not UINT_MAX -- limits.hpp carries why: a fractional place
    // lives in a Number's int exponent, so a wider right half has no Number
    // to be rounded in.
    unsigned long long count = 0;
    if (!dial_count(m, arguments[0], "float_digits",
                    limits::kFloatDigitsLeast, limits::kFloatDigitsMost,
                    &count))
        return false;
    m.retune_float_digits(static_cast<unsigned>(count));
    return true;
}

bool retune_max_depth(eval::Machine &m, const Value *arguments, uint32_t,
                      Value *)
{
    // ANY NUMBER OF BYTES IS A NUMBER OF BYTES -- the dial has no range for
    // config_internal.hpp's reason, and that holds here too: zero refuses
    // the next growth and says so about recursion, which is a working answer.
    unsigned long long bytes = 0;
    if (!dial_count(m, arguments[0], "max_depth", 0, ~0ULL, &bytes))
        return false;
    m.retune_max_depth(bytes);
    return true;
}

bool retune_min_free_mb(eval::Machine &m, const Value *, uint32_t, Value *)
{
    // REFUSED WITH THE REASON, NOT SKIPPED: the watchdog is a detached
    // thread reading the seeded store once a second, and handing it a live
    // value is a synchronization design of its own -- a torn read there
    // kills a healthy process, which is the worst thing a watchdog can do.
    // The mechanism is in place the day that design is taken; until then the
    // row says so instead of letting S0724 imply the path means nothing.
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), "retuning `min_free_mb`",
        "the watchdog reads its floor from the config's store on a thread of "
        "its own, and a live handoff to it is undesigned -- the seeded value "
        "stands for this run"));
    return false;
}

} // namespace

void install_handlers()
{
    using words::NodeId;

    struct Row {
        NodeId path;
        eval::HandlerFn read;
        eval::HandlerFn write;
    };
    // READ AND WRITE INSTALLED TOGETHER, one row per dial, so the two tables
    // cannot disagree about which paths are the dials.
    static constexpr Row rows[] = {
        {NodeId::LIBRARY_SYSTEM_DIVISION_DIGITS, read_division_digits,
         retune_division_digits},
        {NodeId::LIBRARY_SYSTEM_MAX_DEPTH, read_max_depth, retune_max_depth},
        {NodeId::LIBRARY_SYSTEM_MIN_FREE_MB, read_min_free_mb,
         retune_min_free_mb},
        {NodeId::LIBRARY_SYSTEM_FLOAT_DIGITS, read_float_digits,
         retune_float_digits},
    };
    for (const Row &row : rows) {
        eval::Handlers::table().install(
            static_cast<words::PathId>(row.path), {row.read, false, 0, "M15"});
        eval::Assigners::table().install(
            static_cast<words::PathId>(row.path), {row.write, "M15"});
    }
}

} // namespace satellite::system
