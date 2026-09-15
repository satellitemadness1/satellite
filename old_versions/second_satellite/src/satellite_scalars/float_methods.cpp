// `satellite.variable.float`'s methods -- ONE ROW, and M21 is why it exists.
//
// THE FLOAT HAD NO METHOD TABLE AT ALL UNTIL THIS FILE. `1 6 10` was a leaf in
// words.def from M2 until M21: the type landed at M15 with four operations, a
// rounding rule and a dial, and nothing a program could ASK it. That is a
// defensible place to stop -- arithmetic and `display` are most of what a
// float is for -- and M21 found the edge of it by trying to write `Sky::save`.
//
// `Float::to_string()` ALREADY EXISTED IN C++ and had exactly one caller,
// satellite_value/render.cpp, which is the display path. So the value a
// program could see on its screen was one it could not put in a string, write
// to a file, or compare as text: `"x " + a` is S0711 and `write_line(a)` is
// S0713. QUAD's `Sky::save` writes 164 doubles into a `.sky` file with bare
// `operator<<` and was the first mechanism in this tree to need the other
// direction -- while `Sky::load` already worked, because
// `"0.4995225".to_number()` answers. A round-trip available in one direction
// is the shape that made this worth a number rather than a note.
//
// WHAT IT RENDERS IS WHAT `display` RENDERS, and that is the point rather than
// an economy: satellite_float.hpp's contract is "sign, L, '.', then R's
// digits", with one fractional digit minimum so a float never prints as a
// number. A second spelling would let a program's file and a program's screen
// disagree about the same value, which is the drift DESIGN §4.6 exists to
// remove.

#include "satellite_scalars/methods_internal.hpp"

#include "error_reporter/report.hpp"
#include "satellite_float/satellite_float.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::scalars {

namespace {

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

bool float_to_string(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const Float *self = nullptr;
    if (!float_at(m, a, 0, &self))
        return false;
    // THE SAME CHARACTERS `display` WOULD PRINT, encoded raw -- digits, sign
    // and point are all in DESIGN §5's table, so there is no escape to
    // process here any more than there is in a number's.
    *answer = Value::string(encode_raw(self->to_string()));
    return true;
}

} // namespace

bool float_at(eval::Machine &m, const Value *arguments, uint32_t who,
              const Float **out)
{
    const Value &value = arguments[who];
    if (const Flo *held = std::get_if<Flo>(&value)) {
        // A NULL HANDLE IS A POSITIVE ZERO, the defence string_at states for
        // its own arm: Value::floating never builds one, and a method that
        // dereferenced it would be a crash waiting on a producer this module
        // cannot see.
        static const Float zero;
        *out = *held ? held->get() : &zero;
        return true;
    }
    if (value.is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.float`",
        type_name(value)));
    return false;
}

void install_float_methods()
{
    eval::Handlers &table = eval::Handlers::table();
    table.install(
        static_cast<words::PathId>(words::NodeId::VARIABLE_FLOAT_TO_STRING),
        {float_to_string, true, 1, "M21"});
}

} // namespace satellite::scalars
