// THE FOUR CONVERSIONS, OFFERED EVERY WAY A PROGRAM MIGHT REACH FOR THEM --
// the author, 2026-09-12, in the words the motto is usually said in: *"just do
// it all for them! That is our motto -- if we can do it for the user, then we
// do it for them."*
//
// TWO SPELLINGS OF ONE ROW, WHICH IS WHY THIS FILE IS SHORT. `string(n)` and
// `n.string()` are the same handler at two paths -- WORD_NUMBERS §1.5's
// selector and §1.3's call shape -- so the language reads outside-in or
// left-to-right and answers identically. The `to_` rows every type already had
// (`to_string` `1 6 4 6`, `to_number` `1 6 5 1`, `to_hex` `1 6 5 6`,
// `to_binary` `1 6 11 5`) are untouched and still answer: nothing was
// renumbered and nothing retired, which is WORD_NUMBERS §1.2's promise.
//
// THE LINE IS LOSSLESS AGAINST LOSSY AND NOT SCALAR AGAINST NOT. Anything the
// language can convert without dropping information, it does. The two it
// refuses are the two that would invent or discard: a float read as a whole
// number (S0732 -- four roundings, and only the program knows which), and a
// negative or fractional number read as bits (S0733 -- a `BitRun` has "no sign
// and no radix field", so there is no width to pad into that the program did
// not write).
//
// AND A STRING'S BITS DEPEND ON ITS CHARACTERS, WHICH IS THE AUTHOR'S RULE AND
// IS WORTH STATING PLAINLY BECAUSE IT IS THE ONE PLACE HERE THAT READS A
// VALUE'S CONTENT: *"If the user types ONLY 1's and 0's, give them b1010 for
// binary("1010") but otherwise give them the 32 bits."* So `binary("1010")` is
// four bits and `binary("hello")` is forty. What makes this safe where the
// same shape would not be for `+` is that the ANSWER'S TYPE never moves -- it
// is a bit run either way, and only the width differs. `n + s` deciding
// between a number and a string on the contents of `s` would change what every
// line downstream meant; this cannot.

#include "satellite_scalars/methods_internal.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/machine.hpp"
#include "satellite_bits/bits.hpp"
#include "satellite_value/render.hpp"

#include <string>

namespace satellite::scalars {
namespace {

// A whole number of zero or more, as the bits it is made of, shortest first --
// no padding, because a leading zero is a bit the program did not write. Zero
// is one bit wide rather than none: `b` alone is not a literal, and a width of
// zero would print as nothing at all.
bool bits_of_number(eval::Machine &m, const Number &value, bits::BitRun *out)
{
    if (value.is_negative() || !value.is_integer()) {
        m.refuse(errors::make<errors::Code::EVAL_NO_BIT_PATTERN>(
            m.span_of(m.here()), value.to_string(),
            value.is_negative() ? "negative" : "not whole"));
        return false;
    }
    if (value.is_zero()) {
        out->bits.assign(1, false);
        return true;
    }
    // THE ODD BIT IS SUBTRACTED BEFORE THE HALVING, AND THAT IS NOT TIDINESS.
    // `Number::shift_right` is an EXACT division by two -- number_arith.cpp:
    // "TWO MULTIPLICATIONS AND NO DIVISION, WHICH IS WHY NEITHER OF THESE
    // ROUNDS" -- so halving an odd number answers a fraction and the loop below
    // never reaches zero. Written the obvious way it hung on `binary(42)`:
    // 42, 21, 10.5, 5.25, and down forever. Taking the bit off first makes
    // every halving exact, which is the only form in which this language's
    // shift can walk a value down to nothing.
    const Number two(2);
    const Number one(1);
    Number left = value;
    std::vector<bool> backwards;
    while (!left.is_zero()) {
        const bool bit = !Number::modulo(left, two).is_zero();
        backwards.push_back(bit);
        left = Number::shift_right(bit ? Number::sub(left, one) : left, 1);
    }
    out->bits.assign(backwards.rbegin(), backwards.rend());
    return true;
}

// THE SAME VALUE AS HEX, AND THE PADDING IS THE TYPE'S INVARIANT RATHER THAN A
// CHOICE. satellite_bits/bits.hpp: "THE WIDTH IS ALWAYS A MULTIPLE OF FOUR, BY
// CONSTRUCTION", which is exactly what makes `to_binary` total and `to_hex`
// partial. So `binary(42)` is six bits and `hex(42)` is eight -- each minimal
// in its own radix, and neither one rounding the other.
bool hex_of_bits(bits::BitRun run, bits::HexRun *out)
{
    while (run.bits.size() % 4 != 0)
        run.bits.insert(run.bits.begin(), false);
    out->bits = std::move(run);
    return true;
}

// Every byte of the text, eight bits each, most significant first.
bits::BitRun bits_of_bytes(const std::string &text)
{
    bits::BitRun run;
    run.bits.reserve(text.size() * 8);
    for (const char byte : text)
        for (int bit = 7; bit >= 0; bit--)
            run.bits.push_back(((static_cast<unsigned char>(byte) >> bit) & 1) != 0);
    return run;
}

bool only_bit_digits(const std::string &text)
{
    if (text.empty())
        return false;
    for (const char c : text)
        if (c != '0' && c != '1')
            return false;
    return true;
}

// --- the four answers, each from any value that has one ---------------------

bool to_string_of(eval::Machine &m, const Value &value, Value *answer)
{
    if (value.is_string()) {
        *answer = value;
        return true;
    }
    if (!value.is_number() && !value.is_float() && !value.is_binary() &&
        !value.is_hex()) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), "string", "a scalar to render",
            type_name(value)));
        return false;
    }
    *answer = Value::string(encode_raw(satellite::text_of(value)));
    return true;
}

bool to_number_of(eval::Machine &m, const Value &value, Value *answer)
{
    if (value.is_number()) {
        *answer = value;
        return true;
    }
    // A FLOAT ANSWERS ITS EXACT VALUE, AND THE FIRST VERSION OF THIS ARM
    // REFUSED IT ON A PREMISE THAT WAS SIMPLY FALSE. It raised "would drop the
    // fraction", naming `floor`, `ceil`, `round` and `truncate` -- four methods
    // A FLOAT DOES NOT HAVE (its only row before today was `to_string`), to
    // solve a problem it does not have either. `satellite.variable.number` is a
    // DECIMAL bignum, not an integer: `n = 1.5` holds 1.5 and `n + 0.25` is
    // 1.75. And satellite_float.hpp says this direction outright -- to_number
    // is "exact, because L + R is a finite decimal by construction ... the one
    // direction of conversion that needs no rounding rule."
    //
    // WHAT op_to_float's "never this op backwards" MEANS is that there is no
    // SILENT coercion, not that the conversion is lossy. A named one is exactly
    // what DESIGN §1.1 asks for, and `f.number()` is a name.
    if (const Flo *held = std::get_if<Flo>(&value); held != nullptr) {
        *answer = Value::number(*held ? (*held)->to_number() : Number());
        return true;
    }
    if (const bits::BitRun *run = std::get_if<Bin>(&value)
                                      ? &**std::get_if<Bin>(&value)
                                      : nullptr) {
        *answer = Value::number(bits::value_of(*run));
        return true;
    }
    if (const Hex *run = std::get_if<Hex>(&value); run != nullptr && *run) {
        *answer = Value::number(bits::value_of((*run)->bits));
        return true;
    }
    if (value.is_string()) {
        const Str &self = std::get<Str>(value);
        const std::string text = self ? decode(*self) : std::string();
        Number read;
        if (!Number::parse(text, read)) {
            m.refuse(errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                m.span_of(m.here()), text));
            return false;
        }
        *answer = Value::number(std::move(read));
        return true;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), "number", "a string, binary or hex to read",
        type_name(value)));
    return false;
}

bool to_binary_of(eval::Machine &m, const Value &value, Value *answer)
{
    if (value.is_binary()) {
        *answer = value;
        return true;
    }
    if (const Hex *run = std::get_if<Hex>(&value); run != nullptr && *run) {
        *answer = Value::binary((*run)->bits);
        return true;
    }
    if (value.is_number()) {
        bits::BitRun run;
        if (!bits_of_number(m, std::get<Number>(value), &run))
            return false;
        *answer = Value::binary(std::move(run));
        return true;
    }
    if (value.is_string()) {
        const Str &self = std::get<Str>(value);
        const std::string text = self ? decode(*self) : std::string();
        bits::BitRun run;
        if (only_bit_digits(text) && bits::parse_binary(text, run)) {
            *answer = Value::binary(std::move(run));
            return true;
        }
        *answer = Value::binary(bits_of_bytes(text));
        return true;
    }
    // A FLOAT GOES THROUGH ITS EXACT NUMBER, so a whole one has bits and a
    // fractional one is refused by `bits_of_number` -- S0733, "1.5 has no bits
    // -- a bit run holds a whole number of zero or more", which is the true
    // sentence about the real obstacle. The width of a fraction is the thing
    // that does not exist; the conversion to a number never was.
    if (const Flo *held = std::get_if<Flo>(&value); held != nullptr) {
        bits::BitRun run;
        if (!bits_of_number(m, *held ? (*held)->to_number() : Number(), &run))
            return false;
        *answer = Value::binary(std::move(run));
        return true;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), "binary", "a number, string or hex to read",
        type_name(value)));
    return false;
}

bool to_hex_of(eval::Machine &m, const Value &value, Value *answer)
{
    if (value.is_hex()) {
        *answer = value;
        return true;
    }
    Value as_bits;
    if (!to_binary_of(m, value, &as_bits))
        return false;
    const Bin &run = std::get<Bin>(as_bits);
    bits::HexRun made;
    hex_of_bits(run ? *run : bits::BitRun(), &made);
    *answer = Value::hex(std::move(made));
    return true;
}

// --- the handlers, which are the four above with the receiver pulled out -----

bool as_string(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return to_string_of(m, a[0], answer);
}
bool as_number(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return to_number_of(m, a[0], answer);
}
bool as_binary(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return to_binary_of(m, a[0], answer);
}
bool as_hex(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    return to_hex_of(m, a[0], answer);
}

} // namespace

void install_conversions()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        bool binds_receiver;
    };

    // EVERY ROW IS ARITY 1 AND NONE MUTATES, AND THE VALUE ARRIVES AT
    // ARGUMENT 0 EITHER WAY -- which is why one handler serves both spellings.
    // What the two shapes do NOT share is DESIGN §6.4 qualification 2's
    // receiver tag: `n.string()` is a method ON a value and binds one;
    // `string(n)` is a call with one WRITTEN argument and binds none.
    //
    // SETTING IT ON THE BARE ROWS WAS THE FIRST VERSION AND `string(count)`
    // ANSWERED S0718 -- "`string` is a method and is asked on a value, not
    // written out as a path" -- which is dispatch.hpp's tag doing exactly the
    // job it was built for, on a row that had claimed the wrong shape.
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_STRING_OF,     as_string, false},
        {NodeId::VARIABLE_NUMBER_OF,     as_number, false},
        {NodeId::VARIABLE_BINARY_OF,     as_binary, false},
        {NodeId::VARIABLE_HEX_OF,        as_hex,    false},

        {NodeId::VARIABLE_STRING_STRING, as_string, true},
        {NodeId::VARIABLE_STRING_NUMBER, as_number, true},
        {NodeId::VARIABLE_STRING_BINARY, as_binary, true},
        {NodeId::VARIABLE_STRING_HEX, as_hex, true},

        {NodeId::VARIABLE_NUMBER_STRING, as_string, true},
        {NodeId::VARIABLE_NUMBER_NUMBER, as_number, true},
        {NodeId::VARIABLE_NUMBER_BINARY, as_binary, true},
        {NodeId::VARIABLE_NUMBER_HEX, as_hex, true},

        {NodeId::VARIABLE_BINARY_STRING, as_string, true},
        {NodeId::VARIABLE_BINARY_NUMBER, as_number, true},
        {NodeId::VARIABLE_BINARY_BINARY, as_binary, true},
        {NodeId::VARIABLE_BINARY_HEX, as_hex, true},

        {NodeId::VARIABLE_FLOAT_STRING, as_string, true},
        {NodeId::VARIABLE_FLOAT_NUMBER, as_number, true},
        {NodeId::VARIABLE_FLOAT_BINARY, as_binary, true},
        {NodeId::VARIABLE_FLOAT_HEX,    as_hex,    true},

        {NodeId::VARIABLE_HEX_STRING, as_string, true},
        {NodeId::VARIABLE_HEX_NUMBER, as_number, true},
        {NodeId::VARIABLE_HEX_BINARY, as_binary, true},
        {NodeId::VARIABLE_HEX_HEX, as_hex, true},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, row.binds_receiver, 1, "M26", false});
}

} // namespace satellite::scalars
