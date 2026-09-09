// The scalar rows' shared checks, the two module constants, and the one
// install that sums the halves. See satellite_scalars/handlers.hpp.

#include "satellite_scalars/handlers.hpp"

#include "error_reporter/report.hpp"
#include "satellite_scalars/methods_internal.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::scalars {

namespace {

// What the refusal sentences quote: the selector under the caret. The machine
// derives it from the op's own node, so one method misused in two places gets
// two carets and the same word.
std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

const char *spelled(const Value &value)
{
    return type_name(value);
}

// `satellite.bool.true` `1 17 2` and `.false` `1 17 1` -- DESIGN §6.1's
// module constants. A path that evaluates without a call: arity 0, no
// receiver, and the answer is the value the path names. These two rows are
// the whole reason a satellite program can WRITE a bool at all before it has
// compared anything -- comparison has produced them since M9, but until M11
// no literal spelling reached the value.
bool answer_true(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::boolean(true);
    return true;
}

bool answer_false(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::boolean(false);
    return true;
}

} // namespace

bool string_at(eval::Machine &m, const Value *arguments, uint32_t who,
               const SatString **out)
{
    const Value &value = arguments[who];
    if (const Str *text = std::get_if<Str>(&value)) {
        // A NULL HANDLE IS AN EMPTY STRING, defensively -- Value::string never
        // builds one, and a method that crashed on it would be a crash waiting
        // on a producer this module cannot see.
        static const SatString empty;
        *out = *text ? text->get() : &empty;
        return true;
    }
    if (value.is_nothing() && who == 0) {
        // DESIGN §6.4 qualification 3's SECOND state, at its one runtime site:
        // the declaration was the right type -- that is how the selector got
        // its number -- and the slot has never been given a value.
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.string`",
        spelled(value)));
    return false;
}

bool number_at(eval::Machine &m, const Value *arguments, uint32_t who,
               const Number **out)
{
    const Value &value = arguments[who];
    if (const Number *number = std::get_if<Number>(&value)) {
        *out = number;
        return true;
    }
    if (value.is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.number`",
        spelled(value)));
    return false;
}

bool bits_at(eval::Machine &m, const Value *arguments, uint32_t who,
             const bits::BitRun **out)
{
    if (const bits::BitRun *run = as_binary(arguments[who])) {
        // as_binary() answers an empty run for a null handle, so this arm
        // needs no defence of its own -- value.hpp carries the argument.
        *out = run;
        return true;
    }
    if (arguments[who].is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.binary`",
        spelled(arguments[who])));
    return false;
}

bool position_at(eval::Machine &m, const Value *arguments, uint32_t who,
                 unsigned long long *out)
{
    const Number *number = nullptr;
    if (!number_at(m, arguments, who, &number))
        return false;

    // A POSITION IS A WHOLE NUMBER NO LESS THAN ZERO. `at(1.5)` and `at(-1)`
    // are refused rather than rounded or wrapped -- DESIGN §1.1, and the same
    // reasoning S0715's block note records: a refusal can loosen later, and a
    // convention silently adopted is forever.
    long long narrow = 0;
    if (!number->is_integer() || number->is_negative() ||
        !number->to_integer(narrow)) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a whole number no less than 0 that a machine word can hold",
            number->to_string()));
        return false;
    }
    *out = static_cast<unsigned long long>(narrow);
    return true;
}

void install_handlers()
{
    eval::Handlers &table = eval::Handlers::table();

    // NOT RECEIVERS. `satellite.bool` is a namespace and not a value, exactly
    // as `satellite.console` is one row over -- the receiver-binding tag is
    // for methods on VALUES, and a module constant is the opposite of one: no
    // receiver, no arguments, nothing but an answer.
    table.install(static_cast<words::PathId>(words::NodeId::BOOL_TRUE),
                  {answer_true, false, 0, "M11"});
    table.install(static_cast<words::PathId>(words::NodeId::BOOL_FALSE),
                  {answer_false, false, 0, "M11"});

    install_string_methods();
    install_number_methods();
    install_variant_methods();
    install_bits_methods();
}

} // namespace satellite::scalars
