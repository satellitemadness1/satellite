// `satellite.random.seeded` -- the fourth tier, `1 7 13`-`1 7 16`, M21.
//
// A TIER WHOSE WHOLE DIFFERENCE FROM THE OTHER THREE IS THAT IT DOES NOT TAKE
// ANY TIME. DESIGN §11's throwaway window is what separates `fast` from
// `normal` from `ultra` and is the entirety of their difference; this one has
// no window, because the program it exists for -- QUAD's `Rack::draw` -- draws
// from the rack once and calls the generator nineteen more times against a
// 90 ms tick, and the cheapest spinning tier costs about 184 ms per draw on
// this machine.
//
// AND IT IS THE ONLY DRAW A PROGRAM CAN REPLAY, which is the half that made it
// a number rather than a fourth window. QUAD's invariant 8 is determinism --
// quad_core.hpp:77, "the ability to see it twice is the difference between
// science and staring" -- and that is not expressible against a stream seeded
// from the kernel whose state a program can neither read nor set.
//
// A SEPARATE FILE FROM handlers.cpp BECAUSE THE SUBJECT IS SEPARATE, not
// because of an arithmetic: see handlers_internal.hpp, which carries PLAN §3's
// own warning about seams chosen to satisfy a line count.

#include "satellite_random/handlers_internal.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_random/tiers.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::random {

namespace {

// `seeded()` `1 7 16`. Its own sentence and not S0901's: that one offers a
// digit_count this tier does not have.
bool bare_seeded(eval::Machine &m, const Value *, uint32_t, Value *)
{
    m.refuse(errors::make<errors::Code::RANDOM_SEEDED_NEEDS_A_SHAPE>(
        m.span_of(m.here()), callee(m)));
    return false;
}

// Every seeded draw's first question, and it is asked BEFORE the bounds are
// checked: a program that never seeded the stream has a different problem
// from one that wrote a backwards range, and hearing about the range first
// would send it to the wrong line.
bool seeded_ready(eval::Machine &m)
{
    if (stream_is_seeded())
        return true;
    m.refuse(errors::make<errors::Code::RANDOM_NOT_SEEDED>(
        m.span_of(m.here()), callee(m)));
    return false;
}

// The bounds, as numbers that MAY be fractional -- whole_numbers() without its
// second loop. This is the one door in the module that does not demand an
// integer, and tiers.hpp says why the grid makes that answerable.
//
// IT STILL DEMANDS A `satellite.variable.number` AND NOT A FLOAT, which is
// this tier's sharpest edge and is left as the three above it have it: every
// door in this module reads the Number arm. A fractional LITERAL is a Number
// (DESIGN §8.1 -- `0.999` is exact), so `seeded(0.125, 0.875)` is fine and so
// is anything a program typed; what is refused is a value that reached float
// (`n.power(x)` answers one, §8.6's result-type rule), and the sentence it
// gets is S0904's "wants two `satellite.variable.number` arguments", which is
// literally true and reads oddly against a draw whose whole point is
// fractions. `Float::to_number()` is exact and would close it in three lines.
// NOT TAKEN HERE: it changes the argument contract of all four tiers, and that
// is the numbering's and the author's rather than this milestone's.
// MILESTONES/M21.md §4 carries it.
bool plain_numbers(eval::Machine &m, const Value *arguments, uint32_t count,
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
    return true;
}

// `seeded(seed)` `1 7 13`. Answers nothing -- it is a statement about the
// stream and not a draw, so there is no value to hand back and DESIGN §8.7's
// `nothing` is what a call with no answer evaluates to.
bool seeded_seed(eval::Machine &m, const Value *arguments, uint32_t,
                 Value *answer)
{
    const Number *seed = std::get_if<Number>(&arguments[0]);
    if (seed == nullptr || !seed->is_integer() || seed->is_negative()) {
        m.refuse(errors::make<errors::Code::RANDOM_NOT_A_SEED>(
            m.span_of(m.here()), callee(m), text_of(arguments[0])));
        return false;
    }
    long long narrow = 0;
    if (!seed->to_integer(narrow)) {
        m.refuse(errors::make<errors::Code::RANDOM_NOT_A_SEED>(
            m.span_of(m.here()), callee(m), seed->to_string()));
        return false;
    }
    seed_stream(static_cast<unsigned long long>(narrow));
    *answer = Value();
    return true;
}

// `seeded(min, max)` `1 7 14` and its `.range` spelling -- the fractional
// draw. The grid's spacing is `float_digits` read off the MACHINE and not off
// the config, so M15's retune reaches this draw the way it reaches a division:
// a program that turns the dial down draws on a coarser grid from the very
// next call.
bool seeded_range_shape(eval::Machine &m, const Value *arguments, uint32_t,
                        Value *answer)
{
    if (!seeded_ready(m))
        return false;

    // THE WIDTH CHECK IS ON THE UNSCALED SPAN, so the effective ceiling here
    // is MAX_RANDOM_DIGITS less `float_digits` -- the grid multiplies the span
    // by 10^digits before the sampler sees it. A span between the two is
    // caught by seeded_grid() answering false below, which is the same
    // sentence a step further in rather than a crash.
    const Number *bounds[2] = {nullptr, nullptr};
    if (!plain_numbers(m, arguments, 2, "two", bounds) ||
        !ordered(m, *bounds[0], *bounds[1]) ||
        !narrow_enough(m, Number::sub(*bounds[1], *bounds[0])))
        return false;

    Number drawn;
    if (!seeded_grid(*bounds[0], *bounds[1], m.policy().float_digits, drawn)) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_TOO_WIDE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS)));
        return false;
    }
    *answer = Value::number(std::move(drawn));
    return true;
}

// `seeded(min, max, step)` `1 7 15`. The step may be fractional here, so the
// "one or more" floor the three spinning tiers keep (S0909) becomes "greater
// than zero" -- a step of 0.25 is a quarter-grid and a step of 0 is still no
// step at all. The exact-division rule is unchanged and is what keeps `max`
// reachable.
bool seeded_step_shape(eval::Machine &m, const Value *arguments, uint32_t,
                       Value *answer)
{
    if (!seeded_ready(m))
        return false;

    const Number *given[3] = {nullptr, nullptr, nullptr};
    if (!plain_numbers(m, arguments, 3, "three", given))
        return false;

    const Number &low = *given[0];
    const Number &high = *given[1];
    const Number &step = *given[2];

    if (step.is_zero() || step.is_negative()) {
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
    if (!seeded_step(low, high, step, drawn)) {
        m.refuse(errors::make<errors::Code::RANDOM_RANGE_TOO_WIDE>(
            m.span_of(m.here()), callee(m), std::to_string(MAX_RANDOM_DIGITS)));
        return false;
    }
    *answer = Value::number(std::move(drawn));
    return true;
}

} // namespace

void install_seeded_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();

    // THE MILESTONE TAG IS WHAT `satl --words` PRINTS beside a row, so these
    // four say M21 and the twelve in handlers.cpp stay M13.
    const auto row = [&table](NodeId id, eval::HandlerFn fn, uint32_t arity) {
        // NOT A RECEIVER: `satellite.random` is a namespace and not a value.
        table.install(static_cast<words::PathId>(id),
                      eval::Handler{fn, false, arity, "M21"});
    };

    row(NodeId::RANDOM_SEEDED_0, bare_seeded, 0);
    row(NodeId::RANDOM_SEEDED_SEED, seeded_seed, 1);
    row(NodeId::RANDOM_SEEDED_MIN_MAX, seeded_range_shape, 2);
    row(NodeId::RANDOM_SEEDED_MIN_MAX_STEP, seeded_step_shape, 3);
}

} // namespace satellite::random
