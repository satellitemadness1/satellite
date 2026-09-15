// `satellite.variable.hex`'s five methods -- `1 6 11 1` through `1 6 11 5`,
// M19.5's second half. See satellite_scalars/bits_methods.cpp for binary's six
// and satellite_bits/bits.hpp for why a hex run HOLDS a bit run rather than
// deriving from one.
//
// FIVE AND NOT SIX, BECAUSE THERE IS NO `as_number()` HERE. Binary has one at
// `1 6 5 4`; hex had one for a few hours on 2026-09-09 and the author dropped
// it on sight of its answer -- *"x00FF.as_number() = 11111111 looks wrong to
// me"*. It was right to go. `as_number` means "read these characters as an
// ordinary decimal", which binary can answer because `0` and `1` ARE decimal
// digits; hex cannot, there being no decimal spelled `00FF`, so the built
// version read the BIT EXPANSION's characters instead -- 11111111, which no
// program ever wrote. One name over two different questions is worse than one
// question with no name, and words.def carries the numbering consequence.
//
// A SEPARATE FILE AND NOT FIVE MORE ROWS NEXT DOOR, which is the line rule
// (300 lines, 2026-08-28) doing what it is for: bits_methods.cpp was at 150
// before this milestone and both types gained rows, so one file would have
// crossed the line on the day the second radix arrived rather than later, by
// surprise.
//
// EVERY ROW HERE ANSWERS BY ROUTING THROUGH THE BITS, and that is the whole
// design in one sentence. A hex value IS a bit run in a wrapper, so four of
// these five call a `satellite_bits` function that a binary method also calls,
// and the answers cannot drift between the radices because there is one
// implementation of each question. The fifth, `digits()`, is the one thing a
// bit run cannot answer for itself.
//
// NO ROW MUTATES -- bits_methods.cpp's note, and for its reason: every one of
// these answers a value of a DIFFERENT type, so there is nothing to publish
// through the receiver's storage slot (DESIGN §6.4). Nothing here answers a
// hex, because no operation does yet: `+`, `!!` and `[` are OPERATORS, cost no
// path number, and are a later milestone's by the author's call on 2026-09-09.

#include "satellite_scalars/methods_internal.hpp"

#include "satellite_bits/bits.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

namespace satellite::scalars {

namespace {

// `to_number()` `1 6 11 1` -- THE VALUE THE DIGITS STAND FOR.
// `x00FF.to_number()` is 255.
//
// IT READS THE BITS AND IS THEREFORE THE SAME FUNCTION `binary.to_number()`
// CALLS. That is not a saving, it is the correctness argument: one hex digit
// is exactly four bits, so `x00FF` and `b0000000011111111` are the same run,
// and a reader who converted between them and got two different numbers would
// have found a bug in the type's premise. There is one `value_of` and it
// cannot disagree with itself.
//
// THE WIDTH IS NOT CARRIED ACROSS AND CANNOT BE, binary's clause exactly:
// `x0009.to_number()` and `x9.to_number()` are both 9, because a number has no
// width (DESIGN §8.1). `width()` `1 6 11 2` is what answers for it.
bool hex_to_number(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::HexRun *self = nullptr;
    if (!hex_at(m, a, 0, &self))
        return false;
    *answer = Value::number(bits::value_of(self->bits));
    return true;
}

// `width()` `1 6 11 2` -- HOW MANY BITS. `x00FF.width()` is 16, not 4.
//
// BITS AND NOT DIGITS, WHICH IS THE AUTHOR'S CALL OF 2026-09-09 AND HAS A
// CONSEQUENCE WORTH NAMING. `width` means one thing in this language -- how
// many bits are in the run -- so `satellite.variable.file.write(x)`'s "the
// width must be a multiple of eight" is ONE rule that reads the same on both
// types. Had this answered 4, that rule would have silently become
// "a multiple of two" for hex, which is the kind of sentence nobody writes
// down and everybody has to rediscover.
//
// `digits()` `1 6 11 4` IS THE OTHER QUESTION and the author asked for both.
bool hex_width(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::HexRun *self = nullptr;
    if (!hex_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->width())));
    return true;
}

// `to_string()` `1 6 11 3` -- THE CHARACTERS `display` WOULD PRINT, `x` AND
// ALL. `x00FF.to_string()` is "x00FF".
//
// IT MATCHES THE DISPLAY EXACTLY, binary's argument one type over, and here it
// carries a second job: the CASE. A program cannot print what it wrote, since
// case is not stored -- `x00ff` and `x00FF` are one value -- so `display(v)`
// and `display(v.to_string())` agreeing is the only thing that makes the
// printed case predictable at all. Both go through bits.cpp's one function.
bool hex_to_string(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::HexRun *self = nullptr;
    if (!hex_at(m, a, 0, &self))
        return false;
    *answer = Value::string(encode_raw(bits::text_of(*self)));
    return true;
}

// `digits()` `1 6 11 4` -- HOW MANY DIGITS WERE WRITTEN. `x00FF.digits()` is 4.
//
// THE ONE ROW A BIT RUN CANNOT ANSWER FOR THIS TYPE, and the reason `width()`
// could be given to the bits without loss. Four is `width() / 4` and never a
// remainder, because a hex value is built from digits and each contributes
// exactly four bits -- bits.hpp's invariant, which is what makes this division
// a fact rather than a rounding.
//
// `satellite.variable.number.digits` `1 6 4 15` IS THE SAME WORD FOR THE SAME
// QUESTION, one type over, which is why it is spelled this way: M11 gave that
// row to "how many digits does this number have", so the word already meant
// this in this language before either radix had it.
bool hex_digits(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::HexRun *self = nullptr;
    if (!hex_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->digits())));
    return true;
}

// `to_binary()` `1 6 11 5` -- THE SAME VALUE, WEARING THE OTHER RADIX.
// `x00FF.to_binary()` is `b0000000011111111`.
//
// IT CANNOT FAIL, AND ITS TWIN CAN. Every hex digit is four bits, so a hex run
// always has a bit spelling and this row never refuses; `binary.to_hex()`
// `1 6 5 6` is the same conversion backwards and DOES refuse, when the width
// is not a multiple of four. The asymmetry is not an oversight -- it is the
// multiple-of-four invariant seen from each side, and `write(x)`'s
// multiple-of-eight refusal is the same shape one step further out.
//
// IT IS A COPY OF THE BITS AND THAT IS THE POINT OF THE WRAPPER. `self->bits`
// is a `BitRun`, so this is `Value::binary(self->bits)` and nothing is
// converted, computed or rounded -- the two types hold the identical run and
// differ only in which variant arm carries it. THE ANSWER IS STILL NOT EQUAL
// TO THE HEX: `x00FF == x00FF.to_binary()` is FALSE, different arms never
// comparing equal (DESIGN §8), which is the promise §8.5 made as `b1111 == xF`.
bool hex_to_binary(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::HexRun *self = nullptr;
    if (!hex_at(m, a, 0, &self))
        return false;
    *answer = Value::binary(self->bits);
    return true;
}

} // namespace

void install_hex_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER AND EVERY ARITY IS 1, the count including
    // argument 0 -- bits_methods.cpp's note is the rule and these five are its
    // simplest case again: no row here takes a written argument at all.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_HEX_TO_NUMBER, hex_to_number},
        {NodeId::VARIABLE_HEX_WIDTH,     hex_width},
        {NodeId::VARIABLE_HEX_TO_STRING, hex_to_string},
        {NodeId::VARIABLE_HEX_DIGITS,    hex_digits},
        {NodeId::VARIABLE_HEX_TO_BINARY, hex_to_binary},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, 1, "M19.5"});
}

} // namespace satellite::scalars
