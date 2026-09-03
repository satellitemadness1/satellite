// The fifteen `satellite.variable.number` methods, `1 6 4 1` through
// `1 6 4 15` -- installed at M11, specified at M8. See
// satellite_scalars/handlers.hpp for the split: PLAN §8's ledger gives these
// paths to M8 because M8 built everything they answer WITH -- the bignum, the
// sign, the shifts -- and a ledger row means the milestone that makes the path
// answer; the row itself could not exist before M9's table did, and M11 is
// ".number and their methods" running.
//
// THREE ROWS REFUSE NAMING M15, AND WHICH THREE IS A DECISION WITH A RECORD.
// `power`, `truncate` and `sqrt` "cannot be finished before the rounding rule
// is chosen" -- PLAN §8's M15 entry, which also records the two shifts moving
// there and COMING BACK when DESIGN §5.5 answered what a shift means (× 2ⁿ
// and ÷ 2ⁿ, both exact because 2 divides 10), and `modulus` never having
// belonged there at all (§8.6 files it under "exact and bounded, never
// rounds"). So twelve run and three wait, and the three say why.

#include "satellite_scalars/methods_internal.hpp"

#include "error_reporter/report.hpp"
#include "satellite_words/words.hpp"

#include <climits>
#include <string>

namespace satellite::scalars {

namespace {

// A shift count: a whole number from 0 up to what Number's shift takes.
// UINT_MAX shifts of a nonzero number is a value of ~1.3 billion digits, so
// the cap is not a reachable working range -- but a count silently truncated
// into `unsigned` would be a wrong ANSWER, and this is the difference between
// a bound and a lie.
bool shift_count(eval::Machine &m, const Value *arguments, unsigned *out)
{
    unsigned long long wide = 0;
    if (!position_at(m, arguments, 1, &wide))
        return false;
    if (wide > UINT_MAX) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            "a shift count a machine word can hold", std::to_string(wide)));
        return false;
    }
    *out = static_cast<unsigned>(wide);
    return true;
}

bool number_shift_left(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    unsigned by = 0;
    if (!number_at(m, a, 0, &self) || !shift_count(m, a, &by))
        return false;
    *answer = Value::number(Number::shift_left(*self, by));
    return true;
}

bool number_max(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    const Number *other = nullptr;
    if (!number_at(m, a, 0, &self) || !number_at(m, a, 1, &other))
        return false;
    *answer = Value::number(Number::max(*self, *other));
    return true;
}

bool number_min(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    const Number *other = nullptr;
    if (!number_at(m, a, 0, &self) || !number_at(m, a, 1, &other))
        return false;
    *answer = Value::number(Number::min(*self, *other));
    return true;
}

bool number_abs(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    *answer = Value::number(self->abs());
    return true;
}

bool number_clamp(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    const Number *low = nullptr;
    const Number *high = nullptr;
    if (!number_at(m, a, 0, &self) || !number_at(m, a, 1, &low) ||
        !number_at(m, a, 2, &high))
        return false;
    // WHEN low IS ABOVE high THE ANSWER IS low, deterministically --
    // bignum.hpp wrote that rule down at M8 against std::clamp's undefined
    // behaviour, and noted that refusing instead "needs a span, which is the
    // caller's and arrives with the path at M11". The path has arrived and
    // the decision stands as written: a deterministic answer that is already
    // documented beats a refusal added after the fact, and the author can
    // overrule it in one line here.
    *answer = Value::number(Number::clamp(*self, *low, *high));
    return true;
}

bool number_to_string(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    // THE SAME CHARACTERS `display` WOULD PRINT, encoded raw -- digits, sign
    // and point are all in DESIGN §5's table, and there is no escape to
    // process in a number.
    *answer = Value::string(encode_raw(self->to_string()));
    return true;
}

bool number_floor(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    *answer = Value::number(self->floor());
    return true;
}

bool number_ceil(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    *answer = Value::number(self->ceil());
    return true;
}

bool number_round(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    // HALF AWAY FROM ZERO, which is bignum.hpp's documented rule and is not
    // M15's blocker: rounding to an INTEGER has no representation question in
    // it, which is exactly why this row runs while `sqrt` waits.
    *answer = Value::number(self->round());
    return true;
}

bool number_shift_right(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    unsigned by = 0;
    if (!number_at(m, a, 0, &self) || !shift_count(m, a, &by))
        return false;
    *answer = Value::number(Number::shift_right(*self, by));
    return true;
}

bool number_modulus(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    const Number *other = nullptr;
    if (!number_at(m, a, 0, &self) || !number_at(m, a, 1, &other))
        return false;
    if (other->is_zero()) {
        // S0601, THE SAME ROW `%` RAISES -- FORMAT/CXX.md §1's one-place rule:
        // `a.modulus(b)` and `a % b` are one operation with two spellings, and
        // a second sentence would be a second fact about one thing.
        m.refuse(errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
            m.span_of(m.here()), self->to_string()));
        return false;
    }
    *answer = Value::number(Number::modulo(*self, *other));
    return true;
}

// The three that wait on M15's rounding rule, each saying so with the
// milestone's name rather than leaving S0721 to guess.

bool number_power(eval::Machine &m, const Value *, uint32_t, Value *)
{
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), "`power`",
        "a fractional or negative exponent is irrational in general, so it "
        "cannot run before PLAN.md §8's M15 chooses the rounding rule"));
    return false;
}

bool number_truncate(eval::Machine &m, const Value *, uint32_t, Value *)
{
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), "`truncate`",
        "truncating is taking a float's left half, so it waits for the float "
        "itself -- PLAN.md §8 builds it at M15"));
    return false;
}

bool number_sqrt(eval::Machine &m, const Value *, uint32_t, Value *)
{
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), "`sqrt`",
        "a square root is irrational in general, so it cannot run before "
        "PLAN.md §8's M15 chooses the rounding rule"));
    return false;
}

bool number_digits(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Number *self = nullptr;
    if (!number_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->digit_count())));
    return true;
}

} // namespace

void install_number_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER, and the WORD_NUMBERS signatures count it:
    // `max(a, b)` is `a.max(b)` with the receiver as `a` -- DESIGN §6.4's
    // written-out form is exactly that reading -- while `shift_left(n)` names
    // only the written argument. Both spellings land on the same arity rule:
    // the count includes argument 0. NO ROW MUTATES: a number method answers
    // a new number and `n = n.round()` is how a program keeps one, which is
    // the same shape `+` has.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        uint32_t arity;
        const char *milestone;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_NUMBER_SHIFT_LEFT,  number_shift_left,  2, "M11"},
        {NodeId::VARIABLE_NUMBER_MAX,         number_max,         2, "M11"},
        {NodeId::VARIABLE_NUMBER_MIN,         number_min,         2, "M11"},
        {NodeId::VARIABLE_NUMBER_ABS,         number_abs,         1, "M11"},
        {NodeId::VARIABLE_NUMBER_CLAMP,       number_clamp,       3, "M11"},
        {NodeId::VARIABLE_NUMBER_TO_STRING,   number_to_string,   1, "M11"},
        {NodeId::VARIABLE_NUMBER_FLOOR,       number_floor,       1, "M11"},
        {NodeId::VARIABLE_NUMBER_CEIL,        number_ceil,        1, "M11"},
        {NodeId::VARIABLE_NUMBER_ROUND,       number_round,       1, "M11"},
        {NodeId::VARIABLE_NUMBER_POWER,       number_power,       2, "M15"},
        {NodeId::VARIABLE_NUMBER_SHIFT_RIGHT, number_shift_right, 2, "M11"},
        {NodeId::VARIABLE_NUMBER_MODULUS,     number_modulus,     2, "M11"},
        {NodeId::VARIABLE_NUMBER_TRUNCATE,    number_truncate,    1, "M15"},
        {NodeId::VARIABLE_NUMBER_SQRT,        number_sqrt,        1, "M15"},
        {NodeId::VARIABLE_NUMBER_DIGITS,      number_digits,      1, "M11"},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, row.arity, row.milestone});
}

} // namespace satellite::scalars
