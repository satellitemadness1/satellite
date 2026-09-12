// `satellite.random.*` behind the table -- the TWELVE SPINNING rows, three
// tiers by four shapes. See satellite_random/handlers.hpp for why this file is
// the join, and satellite_random/seeded.cpp for the fourth tier M21 added: it
// has no window, and handlers_internal.hpp says why that made it a file rather
// than another hundred and seventy lines here.
//
// EVERY CHECK RUNS BEFORE ANY SPIN. random.hpp's lazy rule: a call that is
// going to fail must not spend three seconds spinning first -- so the width
// checks that v1 left to the sampler are taken here, at the door, and the
// tier_* functions below are reached only by calls that will answer.

#include "satellite_random/handlers.hpp"

#include "satellite_random/handlers_internal.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_random/tiers.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::random {

namespace {


// `<tier>()` `1 7 1`-`1 7 3`, and the bare spelling that folds to the same
// number -- a refusal by design, S0901's note has the decision and its date.
bool bare_tier(eval::Machine &m, const Value *, uint32_t, Value *)
{
    m.refuse(errors::make<errors::Code::RANDOM_NEEDS_A_SHAPE>(
        m.span_of(m.here()), callee(m)));
    return false;
}

// The digit-count argument, checked the way v1 checked it and refused through
// the rows its texts became. True only when `digits` is a whole number a draw
// can honour.
bool digit_count(eval::Machine &m, const Value &given, long long *digits)
{
    const Number *width = std::get_if<Number>(&given);
    if (!width || !width->is_integer()) {
        m.refuse(errors::make<errors::Code::RANDOM_NOT_A_DIGIT_COUNT>(
            m.span_of(m.here()), callee(m), text_of(given)));
        return false;
    }
    if (!width->to_integer(*digits) || *digits < 0 ||
        *digits > MAX_RANDOM_DIGITS) {
        m.refuse(errors::make<errors::Code::RANDOM_DIGITS_OUT_OF_RANGE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS),
            width->to_string()));
        return false;
    }
    return true;
}

// Both range arguments, or all three step arguments, as whole numbers.
// `count_word` is "two" or "three" -- S0904 names the shape it is refusing
// for, because `fast(1, "x")` and `fast(1, 10, "x")` are different calls.
bool whole_numbers(eval::Machine &m, const Value *arguments, uint32_t count,
                   const char *count_word, const Number **out)
{
    for (uint32_t i = 0; i < count; i++) {
        out[i] = std::get_if<Number>(&arguments[i]);
        if (out[i] == nullptr) {
            m.refuse(errors::make<errors::Code::RANDOM_WANTS_NUMBERS>(
                m.span_of(m.here()), callee(m), count_word));
            return false;
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        if (!out[i]->is_integer()) {
            std::string got = out[0]->to_string();
            for (uint32_t j = 1; j < count; j++)
                got += (j + 1 == count ? " and " : ", ") + out[j]->to_string();
            m.refuse(errors::make<errors::Code::RANDOM_WANTS_WHOLE_NUMBERS>(
                m.span_of(m.here()), callee(m), got));
            return false;
        }
    }
    return true;
}



// `<tier>(digits)` `1 7 4`/`1 7 7`/`1 7 10` -- uniform over [0, 10^digits).
bool digits_shape(eval::Machine &m, const Value *arguments, Tier tier,
                  Value *answer)
{
    long long digits = 0;
    if (!digit_count(m, arguments[0], &digits))
        return false;

    Number drawn;
    if (!tier_digits(tier, digits, drawn)) {
        // Unreachable behind the check above, and kept rather than asserted:
        // v1 carried this text for the same belt-and-braces reason, and a
        // sentence beats a crash if the sampler's ceiling ever moves.
        m.refuse(errors::make<errors::Code::RANDOM_COULD_NOT_DRAW>(
            m.span_of(m.here()), callee(m), std::to_string(digits)));
        return false;
    }
    *answer = Value::number(std::move(drawn));
    return true;
}

// `<tier>(min, max)` `1 7 5`/`1 7 8`/`1 7 11`, and the `.range` spelling of
// each -- uniform over [low, high], inclusive at both ends.
bool range_shape(eval::Machine &m, const Value *arguments, Tier tier,
                 Value *answer)
{
    const Number *bounds[2] = {nullptr, nullptr};
    if (!whole_numbers(m, arguments, 2, "two", bounds) ||
        !ordered(m, *bounds[0], *bounds[1]) ||
        !narrow_enough(m, Number::sub(*bounds[1], *bounds[0])))
        return false;

    Number drawn;
    if (!tier_range(tier, *bounds[0], *bounds[1], drawn)) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_TOO_WIDE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS)));
        return false;
    }
    *answer = Value::number(std::move(drawn));
    return true;
}

// `<tier>(min, max, step)` `1 7 6`/`1 7 9`/`1 7 12` -- uniform over
// {min, min + step, ..., max}, specified 2026-09-04: `step` divides
// `max - min` exactly or the call is refused naming the last value the step
// reaches. S0910's note carries the rule and its reason.
bool step_shape(eval::Machine &m, const Value *arguments, Tier tier,
                Value *answer)
{
    const Number *given[3] = {nullptr, nullptr, nullptr};
    if (!whole_numbers(m, arguments, 3, "three", given))
        return false;

    const Number &low = *given[0];
    const Number &high = *given[1];
    const Number &step = *given[2];

    if (Number::compare(step, Number(1)) < 0) {
        m.refuse(errors::make<errors::Code::RANDOM_STEP_NOT_A_STEP>(
            m.span_of(m.here()), callee(m), step.to_string()));
        return false;
    }
    if (!ordered(m, low, high))
        return false;

    const Number span = Number::sub(high, low);
    if (!narrow_enough(m, span))
        return false;

    const Number remainder = Number::modulo(span, step);
    if (!remainder.is_zero()) {
        m.refuse(errors::make<errors::Code::RANDOM_STEP_MISSES>(
            m.span_of(m.here()), callee(m), step.to_string(), low.to_string(),
            Number::sub(high, remainder).to_string(), high.to_string()));
        return false;
    }

    Number drawn;
    if (!tier_step(tier, low, high, step, drawn)) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_TOO_WIDE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS)));
        return false;
    }
    *answer = Value::number(std::move(drawn));
    return true;
}

// The nine drawing rows as functions. A handler is a bare function pointer,
// so the tier cannot ride as a capture; nine two-line wrappers are the price,
// and they read as the table they install. The three bare rows need none --
// bare_tier already has the handler's shape, because the sentence it raises
// quotes the callee and needs no tier.

bool fast_digits(eval::Machine &m, const Value *a, uint32_t, Value *v) { return digits_shape(m, a, Tier::fast, v); }
bool normal_digits(eval::Machine &m, const Value *a, uint32_t, Value *v) { return digits_shape(m, a, Tier::normal, v); }
bool ultra_digits(eval::Machine &m, const Value *a, uint32_t, Value *v) { return digits_shape(m, a, Tier::ultra, v); }

bool fast_range(eval::Machine &m, const Value *a, uint32_t, Value *v) { return range_shape(m, a, Tier::fast, v); }
bool normal_range(eval::Machine &m, const Value *a, uint32_t, Value *v) { return range_shape(m, a, Tier::normal, v); }
bool ultra_range(eval::Machine &m, const Value *a, uint32_t, Value *v) { return range_shape(m, a, Tier::ultra, v); }

bool fast_step(eval::Machine &m, const Value *a, uint32_t, Value *v) { return step_shape(m, a, Tier::fast, v); }
bool normal_step(eval::Machine &m, const Value *a, uint32_t, Value *v) { return step_shape(m, a, Tier::normal, v); }
bool ultra_step(eval::Machine &m, const Value *a, uint32_t, Value *v) { return step_shape(m, a, Tier::ultra, v); }

} // namespace

// --- the three checks seeded.cpp shares ---------------------------------
//
// EXTERNAL LINKAGE AND NOT AN ANONYMOUS NAMESPACE, because the seeded tier
// is a second file now and asks the same three questions. This is the shape
// satellite_scalars/methods_internal.hpp already has over its five method
// files, and PLAN §3 predicts it by name: "headers named <x>_internal.hpp
// existing to hold what an anonymous namespace used to".

// What the refusal sentences call the callee: the spelling op_dispatch
// compiled, which is how `fast.range(1, 100)` is quoted as the program wrote
// it rather than as the number it folded to.
std::string callee(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

// Inclusive at both ends, so low == high is a range of one and is legal;
// low > high is empty, and there is nothing to answer with.
bool ordered(eval::Machine &m, const Number &low, const Number &high)
{
    if (Number::compare(low, high) > 0) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_EMPTY>(
            m.span_of(m.here()), callee(m), low.to_string(), high.to_string()));
        return false;
    }
    return true;
}

// The width check, TAKEN AT THE DOOR rather than left to the sampler: a span
// wider than MAX_RANDOM_DIGITS is refused before the spin it would otherwise
// pay for. The rendered length of the span is its digit count plus at most a
// sign that a checked span cannot have.
bool narrow_enough(eval::Machine &m, const Number &span)
{
    if (span.to_string().size() > static_cast<size_t>(MAX_RANDOM_DIGITS)) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_TOO_WIDE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS)));
        return false;
    }
    return true;
}

void install_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();
    const auto row = [&table](NodeId id, eval::HandlerFn fn, uint32_t arity) {
        // NOT A RECEIVER, ANY OF THEM: `satellite.random.fast(40)` is a path
        // called with written arguments and `random` is a namespace, not a
        // value -- the same tag doing the same job as M10's display row.
        table.install(static_cast<words::PathId>(id),
                      eval::Handler{fn, false, arity, "M13"});
    };

    row(NodeId::RANDOM_FAST_0, bare_tier, 0);
    row(NodeId::RANDOM_NORMAL_0, bare_tier, 0);
    row(NodeId::RANDOM_ULTRA_0, bare_tier, 0);
    row(NodeId::RANDOM_FAST_DIGITS, fast_digits, 1);
    row(NodeId::RANDOM_NORMAL_DIGITS, normal_digits, 1);
    row(NodeId::RANDOM_ULTRA_DIGITS, ultra_digits, 1);
    row(NodeId::RANDOM_FAST_MIN_MAX, fast_range, 2);
    row(NodeId::RANDOM_NORMAL_MIN_MAX, normal_range, 2);
    row(NodeId::RANDOM_ULTRA_MIN_MAX, ultra_range, 2);
    row(NodeId::RANDOM_FAST_MIN_MAX_STEP, fast_step, 3);
    row(NodeId::RANDOM_NORMAL_MIN_MAX_STEP, normal_step, 3);
    row(NodeId::RANDOM_ULTRA_MIN_MAX_STEP, ultra_step, 3);


    // M21's four live in seeded.cpp -- see handlers_internal.hpp.
    install_seeded_handlers();
}

} // namespace satellite::random
