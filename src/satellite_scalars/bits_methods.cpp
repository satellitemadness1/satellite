// `satellite.variable.binary`'s methods -- M19.5's six at `1 6 5 1` through
// `1 6 5 6`, and M26's `clear` at `1 6 5 12`. See
// satellite_scalars/methods_internal.hpp for the receiver check they share,
// satellite_bits/bits.hpp for what a bit run is, and
// satellite_scalars/hex_methods.cpp for the six that mirror these on
// `satellite.variable.hex` `1 6 11`. The conversion rows `1 6 5 7` through
// `1 6 5 11` are one file over, in conversions.cpp, with the rest of their set.
//
// THE LAST TWO ROWS ARRIVED WITH THE SECOND RADIX, 2026-09-09, and the two
// tables were made to line up on purpose: `1 6 5 n` and `1 6 11 n` ask the
// same question for every n, so `digits` is 5 on both and the conversion to
// the OTHER radix is 6 on both. words.def carries the same note where the rows
// are.
//
// NO ROW MUTATED UNTIL M26, and the reason it held for six was structural:
// DESIGN §6.4 makes a mutating method publish through its receiver's storage
// slot, and none of the first six had anything to publish -- every one answers
// a value of a DIFFERENT type. This file's first note finished the thought
// with "there is nothing to mutate until an operation answers a bit run, and
// none does yet". `clear()` is the one that does, answering the empty run of
// the receiver's own type, so it is the first row here to carry `mutates` --
// and the install below carries the column now rather than a comment claiming
// it will never be needed.
//
// STILL NO OPERATORS, which is PLAN §8's "Two numbered paths and no third"
// read at the level below it: `+`, `==`, `!!` and `[` are OPERATORS and
// cost no path number, so a later milestone can give this type any of them
// without minting anything, while a method needs a row in words.def and is
// therefore a decision about the numbering. The first four rows were the
// author's on 2026-09-08 and the last two on 2026-09-09, along with the call
// to leave every operator to a later milestone -- `+` adding, `!!` joining,
// and the whole order-of-operations question decided as one piece rather than
// in the margin of this one.

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

// `digits()` `1 6 5 5` -- HOW MANY DIGITS WERE WRITTEN. `b1010.digits()` is 4.
//
// IT ALWAYS EQUALS `width()` AND IS NOT REDUNDANT, which is worth saying
// because it looks it: one bit is one digit, so these two rows answer the same
// number for every binary value that will ever exist. What earns the row is
// the OTHER type -- `x00FF.width()` is 16 and `x00FF.digits()` is 4 -- so a
// program handed either radix can ask "how many characters did the author
// write" and get an answer without first asking which type it holds. The row
// exists so that the question is askable, not because the answer is news.
//
// THE AUTHOR ASKED FOR IT ON BOTH TYPES, 2026-09-09, and that was the reason.
bool bits_digits(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->width())));
    return true;
}

// `to_hex()` `1 6 5 6` -- THE SAME BITS, WEARING THE OTHER RADIX.
// `b1010.to_hex()` is `xA`.
//
// IT REFUSES WHEN THE WIDTH IS NOT A MULTIPLE OF FOUR, and `b101.to_hex()` is
// that refusal. Four bits are one hex digit and three bits are no hex digit at
// all: padding to four invents a bit the program never wrote, and
// left-aligning invents the same bit while hiding it better. Both are DESIGN
// §1.1's "behind the user's back", so the refusal is the only answer that
// makes nothing up -- `satellite.variable.file.write(x)`'s multiple-of-eight
// rule, one step in.
//
// ITS TWIN CANNOT FAIL. `hex.to_binary()` `1 6 11 6` always answers, because
// every hex digit has four bits; only this direction has a remainder to worry
// about. A program that needs it asks `width()` `1 6 5 2` first.
bool bits_to_hex(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    bits::HexRun made;
    if (!bits::to_hex(*self, made)) {
        // THE SAME REFUSAL `write(x)` MAKES, DOWN TO THE CODE. file_methods.cpp
        // reports its multiple-of-eight failure through EVAL_WRONG_TYPE with
        // the width spelled out rather than through a code of its own, and the
        // two readings of "this run does not divide" should not arrive in a
        // program's output looking like different kinds of problem.
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(m.text_of(m.here())),
            "a `satellite.variable.binary` whose width is a whole number of "
            "hexadecimal digits -- a multiple of 4",
            // The template finishes with "and this one is ...", so this clause
            // starts with the value -- file_methods.cpp's note is the rule.
            bits::text_of(*self) + ", which is " +
                std::to_string(self->width()) + " bits wide"));
        return false;
    }
    *answer = Value::hex(std::move(made));
    return true;
}

// `clear()` `1 6 5 12` -- THE RUN, EMPTIED. `bits.clear()` leaves a run of
// width 0, and it is the one row here that writes back.
//
// IT IS `satellite.variable.string.clear` `1 6 1 15` ON THIS TYPE AND THAT IS
// THE WHOLE ARGUMENT FOR IT. The author asked for it mid-M26 in one sentence
// -- *"The words are supposed to all do the same thing"* -- which is DESIGN
// §4.6's one-word-one-meaning read across the types rather than down a module:
// a reader who has emptied a string should not have to find out whether the
// bit run spells it differently, and a reader who finds no `clear` here learns
// something false about the language rather than about the type.
//
// EMPTY IS WIDTH 0 AND THERE IS NO SECOND READING AVAILABLE. The other
// candidate was "all the bits set to zero, width kept", and §8.5 rules it out
// by making the width part of the value: `b0000` is a four-bit value that is
// not empty, the way `"0000"` is a four-character string that is not empty.
// So the empty value of this type is the run with nothing in it, exactly as
// `SatString()` is the string's.
//
// IT ANSWERS A VALUE `display` CAN PRINT AND THE LEXER CANNOT READ BACK. An
// emptied run prints as `b`, and `b` on its own is not a literal any program
// can write -- lexer.cpp's bits_radix() wants at least one digit. That is the
// same one-way door `satellite.variable.hex` has had since M19.5 for a width
// of zero, and it is not a defect to fix here: the value is reachable, has a
// width, converts and compares. What it has no spelling for is being typed in.
//
// AND IT IS THE FIRST MUTATING ROW ON THIS TYPE, which is why the install
// below grew a column. This file's own note said "NO ROW MUTATES ... there is
// nothing to mutate until an operation answers a bit run, and none does yet."
// One does now, and the note above has been corrected rather than quietly
// left standing.
bool bits_clear(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const bits::BitRun *self = nullptr;
    if (!bits_at(m, a, 0, &self))
        return false;
    *answer = Value::binary(bits::BitRun());
    return true;
}

} // namespace

void install_bits_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    // EVERY ROW BINDS ITS RECEIVER AND EVERY ARITY IS 1, the count including
    // argument 0 -- number_methods.cpp's note is the rule and these are its
    // simplest case: no row here takes a written argument at all.
    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        bool mutates;
        const char *milestone;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_BINARY_TO_NUMBER, bits_to_number, false, "M19.5"},
        {NodeId::VARIABLE_BINARY_WIDTH,     bits_width,     false, "M19.5"},
        {NodeId::VARIABLE_BINARY_TO_STRING, bits_to_string, false, "M19.5"},
        {NodeId::VARIABLE_BINARY_AS_NUMBER, bits_as_number, false, "M19.5"},
        {NodeId::VARIABLE_BINARY_DIGITS,    bits_digits,    false, "M19.5"},
        {NodeId::VARIABLE_BINARY_TO_HEX,    bits_to_hex,    false, "M19.5"},
        {NodeId::VARIABLE_BINARY_CLEAR,     bits_clear,     true,  "M26"},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, 1, row.milestone, row.mutates});
}

} // namespace satellite::scalars
