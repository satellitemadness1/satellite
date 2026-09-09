// `satellite.variable.binary`'s four methods -- `1 6 5 1` through `1 6 5 4`,
// PLAN M19.5. See satellite_scalars/methods_internal.hpp for the receiver
// check they share and satellite_bits/bits.hpp for what a bit run is.
//
// NO ROW MUTATES, which is the number rows' rule one file over and not a
// coincidence: DESIGN §6.4 makes a mutating method publish through its
// receiver's storage slot, and none of these has anything to publish -- every
// one answers a value of a DIFFERENT type. There is nothing to mutate until an
// operation answers a bit run, and none does yet.
//
// FOUR ROWS AND THE TYPE HAS NO OTHERS, which is PLAN §8's "Two numbered paths
// and no third" read at the level below it: `+`, `==` and `[` are OPERATORS
// and cost no path number, so a later milestone can give this type an operator
// without minting anything, while a method needs a row in words.def and is
// therefore a decision about the numbering. These four were the author's,
// 2026-09-08.

#include "satellite_scalars/methods_internal.hpp"

#include "satellite_bits/bits.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

namespace satellite::scalars {

namespace {

// `to_number()` `1 6 5 1` -- THE VALUE THE BITS STAND FOR.
// `b1010.to_number()` is 10.
//
// `to_` IS ALREADY THIS LANGUAGE'S CONVERSION VERB AND THAT IS WHY IT IS THIS
// ONE. `satellite.variable.string.to_number` `1 6 1 13` turns `"42"` into 42
// and `satellite.variable.number.to_string` `1 6 4 6` goes back, so a `to_`
// row answers the SAME VALUE wearing the other type -- and the value a run of
// bits stands for is ten, which is also what every other language answers when
// asked for an integer out of `1010` base two.
//
// AND THE SURPRISING ANSWER MUST NOT SIT BEHIND THE EXPECTED NAME, which is
// the argument that settled it. The author tried it the other way round for
// one sitting on 2026-09-08 and reversed it: a reader of `b1010.to_number()`
// expects 10, and a row that quietly answered 1010 would be a wrong answer
// that looks right -- DESIGN §1.1's "behind the user's back" in the one place
// it is hardest to notice. `as_number()` below is the rare reading, and it
// wears the unusual name for exactly that reason.
//
// THE WIDTH IS NOT CARRIED ACROSS AND CANNOT BE. A number has no width --
// §8.1 makes it one exact unbounded decimal, and §5.5 says the same from the
// other side, "there is no width to shift out of" -- so `b0011.to_number()`
// and `b11.to_number()` are both 3. That is a real loss and it is the answer
// rather than a bug: what the two values differ in is precisely the thing the
// destination type does not have. A program that needs the width back asks
// `width()` `1 6 5 2` before converting.
bool bits_to_number(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::number(bits::value_of(*self));
    return true;
}

// `width()` `1 6 5 2` -- HOW MANY BITS WERE WRITTEN. `b0011.width()` is 4.
//
// THE ONE ROW THAT READS THE PART OF THE VALUE NOTHING ELSE CAN. Every other
// row here answers something a narrower value could also have answered; this
// one answers DESIGN §8.5's whole reason for the type existing, and it is what
// makes the loss the two conversions take recoverable.
bool bits_width(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->width())));
    return true;
}

// `to_string()` `1 6 5 3` -- THE CHARACTERS `display` WOULD PRINT, `b` AND ALL.
//
// IT MATCHES THE DISPLAY EXACTLY AND THAT IS THE POINT OF IT. `display(x)` and
// `display(x.to_string())` print the same thing, which they could not if this
// dropped the prefix -- and two spellings of one value is how a reader ends up
// unable to tell which one a program meant. satellite_value/render.cpp and
// this row go through the one function in bits.cpp, so the two cannot drift.
//
// IT EARNS ITS ROW EVEN THOUGH `display` NEEDS NO CONVERSION, which is worth
// saying because it looks redundant: `+` joins two STRINGS and nothing else
// (DESIGN §6.6), so `"bits: " + b1100` is S0711 and that sentence is
// unwritable without this. It is `satellite.variable.number.to_string`
// `1 6 4 6`'s argument exactly, one type over.
bool bits_to_string(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::string(encode_raw(bits::text_of(*self)));
    return true;
}

// `as_number()` `1 6 5 4` -- THE DIGITS, READ AS IF THEY WERE DECIMAL.
// `b1010.as_number()` is one thousand and ten.
//
// IT IS ONE WORD FROM `to_number()` AND ANSWERS SOMETHING ELSE ENTIRELY, and
// saying so is better than hoping nobody notices: `b1010.to_number()` is 10
// and `b1010.as_number()` is 1010. The distinction is the author's phrasing --
// *to* a number is what the bits MEAN, *as* a number is the writing taken at
// face value -- and this is the second of the two on purpose, being the one
// almost nobody wants.
//
// IT LOSES THE WIDTH TOO, AND BY A DIFFERENT ROUTE. `b0011.as_number()` is 11
// and so is `b11.as_number()`, because a leading zero is no more a surviving
// decimal digit than it is a surviving bit. So neither conversion keeps the
// width and `width()` `1 6 5 2` is the only thing that answers for it.
//
// EVERY BIT RUN HAS ONE AND THE PARSE CANNOT FAIL, a run of `0`s and `1`s
// being a legal decimal by construction -- bits.cpp checks anyway and says why.
bool bits_as_number(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::number(bits::digits_as_number(*self));
    return true;
}

} // namespace

void install_bits_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER AND EVERY ARITY IS 1, the count including
    // argument 0 -- number_methods.cpp's note is the rule and these four are
    // its simplest case: no row here takes a written argument at all.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_BINARY_TO_NUMBER, bits_to_number},
        {NodeId::VARIABLE_BINARY_WIDTH,     bits_width},
        {NodeId::VARIABLE_BINARY_TO_STRING, bits_to_string},
        {NodeId::VARIABLE_BINARY_AS_NUMBER, bits_as_number},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, 1, "M19.5"});
}

} // namespace satellite::scalars
